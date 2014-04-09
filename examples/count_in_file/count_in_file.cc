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

// From a list of jellyfish database, output for each mers how many
// times it occurs in each db. It is very similar to merging (and the
// code is copied/adapted from jellyfish/merge_files.cc).

using jellyfish::file_header;
using jellyfish::RectangularBinaryMatrix;
using jellyfish::mer_dna;
using jellyfish::cpp_array;
typedef std::unique_ptr<binary_reader>           binary_reader_ptr;
typedef std::unique_ptr<text_reader>             text_reader_ptr;
typedef std::unique_ptr<RectangularBinaryMatrix> matrix_ptr;

struct file_info {
  std::ifstream is;
  file_header   header;

  file_info(const char* path) :
    is(path),
    header(is)
  { }
};


struct common_info {
  unsigned int key_len;
  size_t       max_reprobe_offset;
  size_t       size;
  unsigned int out_counter_len;
  std::string  format;
  matrix_ptr   matrix;
};

common_info read_headers(int argc, char* input_files[], cpp_array<file_info>& files) {
  common_info res;

  // Read first file
  files.init(0, input_files[0]);
  if(!files[0].is.good())
    die << "Failed to open input file '" << input_files[0] << "'";

  file_header& h = files[0].header;
  res.key_len            = h.key_len();
  res.max_reprobe_offset = h.max_reprobe_offset();
  res.size               = h.size();
  res.matrix.reset(new RectangularBinaryMatrix(h.matrix()));
  res.format = h.format();
  size_t reprobes[h.max_reprobe() + 1];
  h.get_reprobes(reprobes);
  res.out_counter_len = h.counter_len();

  // Other files must match
  for(int i = 1; i < argc; i++) {
    files.init(i, input_files[i]);
    if(!files[i].is.good())
      die << "Failed to open input file '" << input_files[i] << "'";
    if(res.format != h.format())
      die << "Can't compare files with different formats (" << res.format << ", " << h.format() << ")";
    if(res.key_len != h.key_len())
      die << "Can't compare hashes of different key lengths (" << res.key_len << ", " << h.key_len() << ")";
    if(res.max_reprobe_offset != h.max_reprobe_offset())
      die << "Can't compare hashes with different reprobing strategies";
    if(res.size != h.size())
      die << "Can't compare hash with different size (" << res.size << ", " << h.size() << ")";
    if(*res.matrix != h.matrix())
      die << "Can't compare hash with different hash function";
  }

  return res;
}

template<typename reader_type>
void output_counts(cpp_array<file_info>& files) {
  cpp_array<reader_type> readers(files.size());
  typedef jellyfish::mer_heap::heap<mer_dna, reader_type> heap_type;
  typedef typename heap_type::const_item_t heap_item;
  heap_type heap(files.size());

  // Prime heap
  for(size_t i = 0; i < files.size(); ++i) {
    readers.init(i, files[i].is, &files[i].header);
    if(readers[i].next())
      heap.push(readers[i]);
  }

  heap_item          head      = heap.head();
  mer_dna            key;
  const int          num_files = files.size();
  const reader_type* base      = &readers[0];
  uint64_t           counts[num_files];
  while(heap.is_not_empty()) {
    key = head->key_;
    memset(counts, '\0', sizeof(uint64_t) * num_files);
    do {
      counts[head->it_ - base] = head->val_;
      heap.pop();
      if(head->it_->next())
        heap.push(*head->it_);
      head = heap.head();
    } while(head->key_ == key && heap.is_not_empty());
    std::cout << key;
    for(int i = 0; i < num_files; ++i)
      std::cout << " " << counts[i];
    std::cout << "\n";
  }
}

int main(int argc, char *argv[])
{
  // Check number of input files
  if(argc < 3)
    die << "Usage: " << argv[0] << " file1 file2 ...";

  // Read the header of each input files and do sanity checks.
  cpp_array<file_info> files(argc - 1);
  common_info cinfo = read_headers(argc - 1, argv + 1, files);
  mer_dna::k(cinfo.key_len / 2);

  if(cinfo.format == binary_dumper::format)
    output_counts<binary_reader>(files);
  else if(cinfo.format == text_dumper::format)
    output_counts<text_reader>(files);
  else
    die << "Format '" << cinfo.format << "' not supported\n";

  return 0;
}

  // // populate the initial heap
  // typedef jellyfish::heap_t<hash_index> hheap_t;
  // hheap_t heap(num_hashes);

  // for(int h = 0; h < num_hashes; ++h) {
  //   if(iters[h].next())
  //     heap.push(iters[h]);
  // }

  // // output counts in each files
  // hheap_t::const_item_t head = heap.head();
  // uint64_t* counts = new uint64_t[num_hashes];
  // const uint_t mer_len = rklen / 2;
  // char mer_str[mer_len + 1];

  // while(heap.is_not_empty()) {
  //   uint64_t key = head->key;
  //   memset(counts, '\0', sizeof(uint64_t) * num_hashes);
  //   do {
  //     counts[head->it->index] = head->val;

  //     heap.pop();
  //     if(head->it->next())
  //       heap.push(*head->it);
  //     head = heap.head();
  //   } while(heap.is_not_empty() && head->key == key);

  //   // Print counts
  //   jellyfish::parse_dna::mer_binary_to_string(key, mer_len, mer_str);
  //   std::cout << mer_str;
  //   for(int i = 0; i < num_hashes; ++i)
  //     std::cout << " " << counts[i];
  //   std::cout << "\n";
  // }
  // std::cout << std::flush;
