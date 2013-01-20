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
#include <string>

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_heap.hpp>
#include <jellyfish/jellyfish.hpp>
#include <jellyfish/rectangular_binary_matrix.hpp>
#include <jellyfish/cpp_array.hpp>
#include <sub_commands/merge_main_cmdline.hpp>

using jellyfish::file_header;
using jellyfish::RectangularBinaryMatrix;
using jellyfish::mer_dna;
using jellyfish::cpp_array;
typedef std::auto_ptr<binary_reader> binary_reader_ptr;
typedef std::auto_ptr<text_reader> text_reader_ptr;

struct file_info {
  std::ifstream is;
  file_header   header;

  file_info(const char* path) :
  is(path),
  header(is)
  { }
};
typedef std::auto_ptr<RectangularBinaryMatrix> matrix_ptr;

template<typename reader_type, typename writer_type>
void do_merge(cpp_array<file_info>& files, std::ostream& out, writer_type& writer) {
  cpp_array<reader_type> readers(files.size());
  typedef jellyfish::mer_heap::heap<mer_dna, reader_type> heap_type;
  typedef typename heap_type::const_item_t heap_item;
  heap_type heap(files.size());

  for(size_t i = 0; i < files.size(); ++i) {
    readers.init(i, files[i].is, &files[i].header);
    if(readers[i].next())
      heap.push(readers[i]);
  }

  heap_item head = heap.head();
  mer_dna   key;
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
    writer.write(out, key, sum);
  }
}

int merge_main(int argc, char *argv[])
{
  file_header out_header;
  out_header.fill_standard();
  out_header.set_cmdline(argc, argv);

  merge_main_cmdline args(argc, argv);
  unsigned int       key_len            = 0;
  size_t             max_reprobe_offset = 0;
  size_t             size               = 0;
  std::string        format;
  matrix_ptr         matrix;

  cpp_array<file_info> files(args.input_arg.size());

  // create an iterator for each hash file
  for(size_t i = 0; i < files.size(); i++) {
    files.init(i, args.input_arg[i]);
    if(!files[i].is.good())
      die << "Failed to open input file '" << args.input_arg[i];

    file_header& h = files[i].header;
    if(i == 0) {
      key_len            = h.key_len();
      max_reprobe_offset = h.max_reprobe_offset();
      size               = h.size();
      matrix.reset(new RectangularBinaryMatrix(h.matrix()));
      out_header.size(size);
      out_header.key_len(key_len);
      format = h.format();
      out_header.matrix(*matrix);
      out_header.max_reprobe(h.max_reprobe());
      size_t reprobes[h.max_reprobe() + 1];
      h.get_reprobes(reprobes);
      out_header.set_reprobes(reprobes);
    } else {
      if(format != h.format())
        die << "Can't merge files with different formats " << i << " " << args.input_arg[i] << " " << format << " " << h.format();
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
  out_header.counter_len(args.out_counter_len_arg);
  out_header.format(format);
  mer_dna::k(key_len / 2);

  std::ofstream out(args.output_arg);
  out_header.write(out);

  if(!format.compare(binary_dumper::format)) {
    binary_writer writer(args.out_counter_len_arg, key_len);
    do_merge<binary_reader, binary_writer>(files, out, writer);
  } else if(!format.compare(text_dumper::format)) {
    text_writer writer;
    do_merge<text_reader, text_writer>(files, out, writer);
  } else {
    die << "Unknown format '" << format << "'";
  }
  out.close();

  return 0;
}
