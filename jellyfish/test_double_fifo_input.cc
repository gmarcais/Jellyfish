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

#include <jellyfish/err.hpp>
#include <jellyfish/parse_dna.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/backtrace.hpp>
#include <stdlib.h>

class display_kmer {
  int           _k;
  unsigned long count;
public:
  typedef unsigned long val_type;
  display_kmer(int k) : _k(k), count(0) {}

  void add(uint64_t mer, int i, val_type *oval = 0) {
    ++count;
  }
  void inc(uint64_t mer) {
    // char mer_str[_k+1];
    // jellyfish::parse_dna::mer_binary_to_string(mer, _k, mer_str);
    // std::cout << mer_str << std::endl;
    ++count;
  }
  
  display_kmer *operator->() { return this; }
  unsigned long get_count() { return count; }
};


class parser : public thread_exec {
  jellyfish::parse_dna _parser;
  unsigned long        count;

public:
  parser(int argc, const char *argv[]) :
    _parser(argv, argv + argc, 25, 100, 1024*10),
    count(0)
  { }

  void start(int id) {
    jellyfish::parse_dna::thread tparse = _parser.new_thread();

    display_kmer display(25);
    tparse.parse(display);
    atomic::gcc::fetch_add(&count, display.get_count());
  }

  unsigned long get_count() { return count; }
};


int main(int argc, const char *argv[])
{
  show_backtrace();

  if(argc != 3)
    die << "Need the number of threads and file name";

  parser parse(argc - 2, argv + 2);
  parse.exec_join(atoi(argv[1]));
  std::cout << "Get count " << parse.get_count() << std::endl;
  
  return 0;
}
