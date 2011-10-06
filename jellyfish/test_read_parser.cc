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

#include <jellyfish/parse_read.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <jellyfish/dbg.hpp>
#include <jellyfish/backtrace.hpp>

int main(int argc, char *argv[])
{
  show_backtrace();
  jellyfish::parse_read parser(argv + 1, argv + argc, 100);
  jellyfish::parse_read::thread stream = parser.new_thread();

  jellyfish::read_parser::read_t *read;
  while((read = stream.next_read())) {
    unsigned long count = 0;
    for(const char *base = read->seq_s; base < read->seq_e; ++base) {
      switch(*base) {
      case 'a': case 'A':
      case 'c': case 'C':
      case 'g': case 'G':
      case 't': case 'T':
        ++count;

      default: break; //skip
      }
    }
    std::cout << "'" << std::string(read->header, read->hlen) << "' "
              << count << "\n";
  }
  std::cout << std::flush;

  return 0;
}
