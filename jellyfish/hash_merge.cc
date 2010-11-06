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
#include <argp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "misc.hpp"
#include "mer_counting.hpp"
#include "compacted_hash.hpp"
#include "heap.hpp"

#define MAX_KMER_SIZE 32

/**
 * Command line processing
 **/
static char doc[] = "Merge tables created by 'jellyfish count'";
static char args_doc[] = "table1 table2 ...";

enum {
  OPT_VAL_LEN = 1024,
  OPT_OBUF_SIZE
};

static struct argp_option options[] = {
  {"fasta",     'f',    0,  0,  "Print k-mers in fasta format (false)"},
  {"output",    'o',  "FILE", 0, "Output file"},
  {"out-counter-len", OPT_VAL_LEN, "LEN", 0, "Length (in bytes) of counting field in output"},
  {"out-buffer-size", OPT_OBUF_SIZE,  "SIZE", 0, "Size of output buffer per thread"},
  {0}
};

struct arguments {
  bool          fasta;
  const char *  output;
  unsigned long out_counter_len;
  size_t        out_buffer_size;
};


/**
 * Parse the command line arguments
 **/
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;
  error_t error;

#define ULONGP(field) \
  error = parse_long(arg, &std::cerr, &arguments->field);       \
  if(error) return(error); else break;

#define FLAG(field) arguments->field = true; break;
#define STRING(field) arguments->field = arg; break;

  switch(key) {
  case 'f': FLAG(fasta);
  case 'o': STRING(output);
  case OPT_VAL_LEN: ULONGP(out_counter_len);
  case OPT_OBUF_SIZE: ULONGP(out_buffer_size);
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };



class ErrorWriting : public std::exception {
  std::string msg;
public:
  ErrorWriting(const std::string _msg) : msg(_msg) {}
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

int merge_main(int argc, char *argv[]) {

  // Process the command line
  struct arguments cmdargs;
  int arg_st = 1;
  cmdargs.fasta = false;
  cmdargs.out_counter_len = 4;
  cmdargs.out_buffer_size = 10000000UL;
  cmdargs.output = "mer_counts_merged.hash";
  argp_parse(&argp, argc, argv, 0, &arg_st, &cmdargs);

  int i;
  unsigned int rklen = 0;
  size_t max_reprobe = 0;
  size_t hash_size = 0;
  SquareBinaryMatrix hash_matrix;
  SquareBinaryMatrix hash_inverse_matrix;

  // compute the number of hashes we're going to read
  int num_hashes = argc - arg_st;
  if(num_hashes <= 0) {
    fprintf(stderr, "No hash files given\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  // this is our row of iterators
  hash_reader_t iters[num_hashes];
 
  // create an iterator for each hash file
  for(i = 0; i < num_hashes; i++) {
    char *db_file;

    // open the hash database
    db_file = argv[i + arg_st];
    try {
      iters[i].initialize(db_file, cmdargs.out_buffer_size);
    } catch(exception *e) {
      die("Can't open k-mer database: %s\n", e->what());
    }

    unsigned int len = iters[i].get_key_len();
    if(rklen != 0 && len != rklen)
      die("Can't merge hashes of different key lengths\n");
    rklen = len;

    uint64_t rep = iters[i].get_max_reprobe();
    if(max_reprobe != 0 && rep != max_reprobe)
      die("Can't merge hashes with different reprobing stratgies\n");
    max_reprobe = rep;

    size_t size = iters[i].get_size();
    if(hash_size != 0 && size != hash_size)
      die("Can't merge hash with different size\n");
    hash_size = size;

    if(hash_matrix.get_size() < 0) {
      hash_matrix = iters[i].get_hash_matrix();
      hash_inverse_matrix = iters[i].get_hash_inverse_matrix();
    } else {
      SquareBinaryMatrix new_hash_matrix = iters[i].get_hash_matrix();
      SquareBinaryMatrix new_hash_inverse_matrix = 
	iters[i].get_hash_inverse_matrix();
      if(new_hash_matrix != hash_matrix || 
	 new_hash_inverse_matrix != hash_inverse_matrix)
	die("Can't merge hash with different hash function\n");
    }
  }

  if(rklen == 0)
    die("No valid hash tables found.\n");

  fprintf(stderr, "mer length  = %d\n", rklen / 2);
  fprintf(stderr, "hash size   = %ld\n", hash_size);
  fprintf(stderr, "num hashes  = %d\n", num_hashes);
  fprintf(stderr, "max reprobe = %ld\n", max_reprobe);
  fprintf(stderr, "heap size = %d\n", num_hashes);

  // populate the initial heap
  typedef jellyfish::heap_t<hash_reader_t> hheap_t;
  hheap_t heap(num_hashes);
  heap.fill(iters, iters + num_hashes);

  if(heap.is_empty()) 
    die("Hashes contain no items.");

  // open the output file
  std::ofstream out(cmdargs.output);
  size_t nb_records = cmdargs.out_buffer_size /
    (bits_to_bytes(rklen) + bits_to_bytes(8 * cmdargs.out_counter_len));
  printf("buffer size %ld nb_records %ld\n", cmdargs.out_buffer_size,
         nb_records);
  writer_info info;
  info.nb_buffers = 4;
  info.out = &out;
  info.done = false;
  info.buffers = new writer_buffer[info.nb_buffers];
  for(uint_t j = 0; j < info.nb_buffers; j++) {
    info.buffers[j].full = false;
    info.buffers[j].writer.initialize(nb_records, rklen,
                                      8 * cmdargs.out_counter_len, &iters[0]);
  }
  info.buffers[0].writer.write_header(&out);

  pthread_t writer_thread;
  pthread_create(&writer_thread, NULL, writer_function, &info);

  fprintf(stderr, "out kmer len = %ld bytes\n", info.buffers[0].writer.get_key_len_bytes());
  fprintf(stderr, "out val len = %ld bytes\n", info.buffers[0].writer.get_val_len_bytes());

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
  //printf("main waiting %ld writer waiting %ld\n", waiting, *writer_waiting);
  //delete writer_waiting;
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
