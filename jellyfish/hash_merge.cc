/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/compacted_hash.hpp>
#include <jellyfish/heap.hpp>
#include <jellyfish/hash_merge_cmdline.hpp>

#define MAX_KMER_SIZE 32

class ErrorWriting : public std::exception {
  std::string msg;
public:
  ErrorWriting(const std::string &_msg) : msg(_msg) {}
  virtual ~ErrorWriting() throw() {}
  virtual const char* what() const throw() {
    return msg.c_str();
  }
};

struct writer_buffer {
  volatile bool full;
  hash_writer_t writer;
};
struct writer_info {
  locks::pthread::cond  cond;
  volatile bool         done;
  uint_t                nb_buffers;
  std::ostream         *out;
  writer_buffer        *buffers;
};

void *writer_function(void *_info) {
  writer_info *info = (writer_info *)_info;
  uint_t buf_id = 0;
  uint64_t waiting = 0;

  while(true) {
    info->cond.lock();
    while(!info->buffers[buf_id].full) {
      if(info->done) {
        info->cond.unlock();
        return new uint64_t(waiting);
      }
      waiting++;
      info->cond.wait();
    }
    info->cond.unlock();

    info->buffers[buf_id].writer.dump(info->out);

    info->cond.lock();
    info->buffers[buf_id].full = false;
    info->cond.signal();
    info->cond.unlock();

    buf_id = (buf_id + 1) % info->nb_buffers;
  }
}

int merge_main(int argc, char *argv[])
{
  merge_args args(argc, argv);

  int                i;
  unsigned int       rklen       = 0;
  size_t             max_reprobe = 0;
  size_t             hash_size   = 0;
  SquareBinaryMatrix hash_matrix;
  SquareBinaryMatrix hash_inverse_matrix;

  // compute the number of hashes we're going to read
  int num_hashes = args.input_arg.size();

  // this is our row of iterators
  hash_reader_t iters[num_hashes];
 
  // create an iterator for each hash file
  for(i = 0; i < num_hashes; i++) {
    // open the hash database
    const char *db_file = args.input_arg[i];

    try {
      iters[i].initialize(db_file, args.out_buffer_size_arg);
    } catch(std::exception *e) {
      die << "Can't open k-mer database: " << e;
    }

    unsigned int len = iters[i].get_key_len();
    if(rklen != 0 && len != rklen)
      die << "Can't merge hashes of different key lengths";
    rklen = len;

    uint64_t rep = iters[i].get_max_reprobe();
    if(max_reprobe != 0 && rep != max_reprobe)
      die << "Can't merge hashes with different reprobing stratgies";
    max_reprobe = rep;

    size_t size = iters[i].get_size();
    if(hash_size != 0 && size != hash_size)
      die << "Can't merge hash with different size";
    hash_size = size;

    if(hash_matrix.get_size() == 0) {
      hash_matrix = iters[i].get_hash_matrix();
      hash_inverse_matrix = iters[i].get_hash_inverse_matrix();
    } else {
      SquareBinaryMatrix new_hash_matrix = iters[i].get_hash_matrix();
      SquareBinaryMatrix new_hash_inverse_matrix = 
	iters[i].get_hash_inverse_matrix();
      if(new_hash_matrix != hash_matrix || 
	 new_hash_inverse_matrix != hash_inverse_matrix)
	die << "Can't merge hash with different hash function";
    }
  }

  if(rklen == 0)
    die << "No valid hash tables found";

  typedef jellyfish::heap_t<hash_reader_t> hheap_t;
  hheap_t heap(num_hashes);

  if(args.verbose_flag)
    std::cerr << "mer length  = " << (rklen / 2) << "\n"
              << "hash size   = " << hash_size << "\n"
              << "num hashes  = " << num_hashes << "\n"
              << "max reprobe = " << max_reprobe << "\n"
              << "heap capa   = " << heap.capacity() << "\n"
              << "matrix      = " << hash_matrix.xor_sum() << "\n"
              << "inv_matrix  = " << hash_inverse_matrix.xor_sum() << "\n";

  // populate the initial heap
  for(int h = 0; h < num_hashes; ++h) {
    if(iters[h].next())
      heap.push(iters[h]);
  }
  assert(heap.size() == heap.capacity());

  if(heap.is_empty()) 
    die << "Hashes contain no items.";

  // open the output file
  std::ofstream out(args.output_arg.c_str());
  size_t nb_records = args.out_buffer_size_arg /
    (bits_to_bytes(rklen) + bits_to_bytes(8 * args.out_counter_len_arg));
  if(args.verbose_flag)
    std::cerr << "buffer size " << args.out_buffer_size_arg 
              << " nb_records " << nb_records << "\n";
  writer_info info;
  info.nb_buffers = 4;
  info.out = &out;
  info.done = false;
  info.buffers = new writer_buffer[info.nb_buffers];
  for(uint_t j = 0; j < info.nb_buffers; j++) {
    info.buffers[j].full = false;
    info.buffers[j].writer.initialize(nb_records, rklen,
                                      8 * args.out_counter_len_arg, &iters[0]);
  }
  info.buffers[0].writer.write_header(&out);

  pthread_t writer_thread;
  pthread_create(&writer_thread, NULL, writer_function, &info);

  if(args.verbose_flag)
    std::cerr << "out kmer len = " << info.buffers[0].writer.get_key_len_bytes() << " bytes\n"
              << "out val len  = " << info.buffers[0].writer.get_val_len_bytes() << " bytes\n";

  uint_t buf_id = 0;
  uint64_t waiting = 0;
  hheap_t::const_item_t head = heap.head();
  while(heap.is_not_empty()) {
    uint64_t key = head->key;
    uint64_t sum = 0;
    do {
      sum += head->val;
      heap.pop();
      if(head->it->next())
        heap.push(*head->it);
      head = heap.head();
    } while(heap.is_not_empty() && head->key == key);

    while(!info.buffers[buf_id].writer.append(key, sum)) {
      info.cond.lock();
      info.buffers[buf_id].full = true;
      buf_id = (buf_id + 1) % info.nb_buffers;
      while(info.buffers[buf_id].full) { waiting++; info.cond.wait(); }
      info.cond.signal();
      info.cond.unlock();
    }
  }
  info.cond.lock();
  //  while(info.buffers[buf_id].full) { waiting++; info.cond.wait(); }
  info.buffers[buf_id].full = true;
  info.done = true;
  info.cond.signal();
  info.cond.unlock();

  //uint64_t *writer_waiting;
  pthread_join(writer_thread, NULL);

  uint64_t unique = 0, distinct = 0, total = 0, max_count = 0;
  for(uint_t j = 0; j < info.nb_buffers; j++) {
    unique += info.buffers[j].writer.get_unique();
    distinct += info.buffers[j].writer.get_distinct();
    total += info.buffers[j].writer.get_total();
    if(info.buffers[j].writer.get_max_count() > max_count)
      max_count = info.buffers[j].writer.get_max_count();
  }
  info.buffers[0].writer.update_stats_with(&out, unique, distinct, total, max_count);
  out.close();
  return 0;
}
