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

void jellyfish::parse_read::fill() {
  read_parser::reads_t *new_seq = write_next();
  
  while(new_seq) {
    new_seq->file = fparser;
    bool input_eof = !fparser->next_reads(new_seq);
    if(new_seq->nb_reads > 0) {
      new_seq->link();
      write_release(new_seq);
      new_seq = write_next();
    }
    if(input_eof) {
      fparser->unlink();
      if(++current_file == files.end()) {
        close();
        break;
      }
      fparser = read_parser::new_parser(*current_file);
      fparser->link();
    }
  }
}
