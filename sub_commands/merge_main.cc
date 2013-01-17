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
#include <memory>

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_heap.hpp>
#include <jellyfish/jellyfish.hpp>
#include <jellyfish/rectangular_binary_matrix.hpp>
#include <sub_commands/merge_main_cmdline.hpp>

using jellyfish::file_header;
using jellyfish::RectangularBinaryMatrix;
using jellyfish::mer_dna;
typedef std::auto_ptr<binary_reader> reader_ptr;

struct file_info {
  std::ifstream is;
  file_header   header;
  reader_ptr    reader;

  file_info(const char* path) :
  is(path),
  header(is)
  { }

  void start_reader() { reader.reset(new binary_reader(is, &header)); }
};
typedef std::auto_ptr<RectangularBinaryMatrix> matrix_ptr;
typedef jellyfish::mer_heap::heap<mer_dna, binary_reader> heap_type;

int merge_main(int argc, char *argv[])
{
  file_header header;
  header.fill_standard();
  header.set_cmdline(argc, argv);
  // TODO: need to refactor binary_dumper and sorted_dumper. Can't be use here -> code duplication
  header.format(binary_dumper::format);

  merge_main_cmdline args(argc, argv);
  unsigned int       key_len            = 0;
  size_t             max_reprobe_offset = 0;
  size_t             size               = 0;
  matrix_ptr         matrix;

  int num_hashes = args.input_arg.size();
  std::pair<file_info*, std::ptrdiff_t> files_ = std::get_temporary_buffer<file_info>(num_hashes);
  if(files_.second < num_hashes)
    die << "Out of memory";
  file_info* files = files_.first;

  // create an iterator for each hash file
  for(int i = 0; i < num_hashes; i++) {
    new (&files[i]) file_info(args.input_arg[i]);
    if(!files[i].is.good())
      die << "Failed to open input file '" << args.input_arg[i];

    file_header& h = files[i].header;
    if(i == 0) {
      key_len            = h.key_len();
      max_reprobe_offset = h.max_reprobe_offset();
      size               = h.size();
      matrix.reset(new RectangularBinaryMatrix(h.matrix()));
      header.size(size);
      header.key_len(key_len);
      header.matrix(*matrix);
      header.max_reprobe(header.max_reprobe());
      size_t reprobes[h.max_reprobe() + 1];
      h.get_reprobes(reprobes);
      header.set_reprobes(reprobes);
    } else {
      if(h.key_len() != key_len)
        die << "Can't merge hashes of different key lengths";
      if(h.max_reprobe_offset() != max_reprobe_offset)
        die << "Can't merge hashes with different reprobing strategies";
      if(h.size() != size)
        die << "Can't merge hash with different size";
      if(h.matrix() != *matrix)
        die << "Can't merge hash with different hash function";
    }
  }
  header.counter_len(args.out_counter_len_arg);
  mer_dna::k(key_len / 2);
  heap_type heap(num_hashes);

  // if(args.verbose_flag)
  //   std::cerr << "mer length  = " << (rklen / 2) << "\n"
  //             << "hash size   = " << hash_size << "\n"
  //             << "num hashes  = " << num_hashes << "\n"
  //             << "max reprobe = " << max_reprobe << "\n"
  //             << "heap capa   = " << heap.capacity() << "\n"
  //             << "matrix      = " << hash_matrix.xor_sum() << "\n"
  //             << "inv_matrix  = " << hash_inverse_matrix.xor_sum() << "\n";

  // open the output file
  std::ofstream out(args.output_arg);
  header.write(out);

  // populate the initial heap
  for(int h = 0; h < num_hashes; ++h) {
    files[h].start_reader();
    if(files[h].reader->next())
      heap.push(*files[h].reader);
  }
  //  assert(heap.size() == heap.capacity());

  heap_type::const_item_t head        = heap.head();
  mer_dna                  key;
  int                      out_key_len = key_len / 8 + (key_len % 8 != 0);
  uint64_t                 max_val     = ((uint64_t)1 << (args.out_counter_len_arg * 8)) - 1;
  while(heap.is_not_empty()) {
    key = head->key_;
    uint64_t sum = 0;
    do {
      sum += head->val_;
      heap.pop();
      if(head->it_->next())
        heap.push(*head->it_);
      head = heap.head();
    } while(head->key_ == key && heap.is_not_empty());
    out.write((const char*)key.data(), out_key_len);
    uint64_t v = std::min(max_val, sum);
    out.write((const char*)&v, args.out_counter_len_arg);
  }

  out.close();
  std::return_temporary_buffer(files); // TODO: memory is leaked here. Contained objects are not deleted

  return 0;
}
