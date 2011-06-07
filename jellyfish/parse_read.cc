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

jellyfish::parse_read(int nb_files, char *argv[], unsigned int nb_buffers) :
  double_fifo_input(nb_buffers), files(argv, argv + nb_files),
  current_file(files.begin()),
  fparser(file_parser::new_file_parser_read(*current_file)
{ }

jellyfish::~parse_read() { }

void jellyfish::parse_read::fill() {
  reads_t *new_seq = wq.dequeue();
  
  while(new_seq) {
    bool input_eof = !fparser->parse(new_seq);
    if(new_seq->nb_reads > 0) {
      rq.enqueue(new_seq);
      new_seq = wq.dequeue();
    }
    if(input_eof) {
      delete fparser;
      if(++current_file == files.end()) {
        rq.close();
        break;
      }
      fparser = file_parser::new_file_parser_read(*current_file);
    }
  }
}
