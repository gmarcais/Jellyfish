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

#include <jellyfish/fastq_read_parser.hpp>

fastq_read_parser::fastq_read_parser(int nb_files, char *argv[], unsigned int nb_buffers) :
  mapped_files(nb_files, argv, true),
  current_file(mapped_files.begin()),
  reader(0),
  rq(nb_buffers), wq(nb_buffers)
{
  buffers = new seq[nb_buffers];
  memset(buffers, '\0', sizeof(struct seq) * nb_buffers);
  for(unsigned int i = 0; i < nb_buffers; i++)
    wq.enqueue(&buffers[i]);

  current_file->map();
  current_file->will_need();
  map_base = current_file->base();
  current  = map_base;
  map_end  = current_file->end();
}

bool fastq_read_parser::thread::next_sequence() {
  if(rq->is_low() && !rq->is_closed()) {
    parser->read_sequence();
  }
  while(!(sequence = rq->dequeue())) {
    if(rq->is_closed())
      return false;
    parser->read_sequence();
  }
  return true;
}

void fastq_read_parser::_read_sequence() {
  seq *new_seq;
  int  nb_reads;

  // New lines are not allowed in sequence or quals
#define BREAK_AT_END if(current >= map_end) break;
  while((new_seq = wq.dequeue()) != 0) {
    nb_reads                      = 0;
    while(nb_reads < max_nb_reads && current < map_end) {
      read *cread                 = &new_seq->reads[nb_reads];

      while(true) {
        // Find & skip header
        while(*current++ != '@')
          BREAK_AT_END;
        BREAK_AT_END;
        cread->header = current;
        while(*current++ != '\n')
          BREAK_AT_END;
        BREAK_AT_END;
        cread->hlen = current - cread->header - 1;
        cread->seq_s  = current;
        
        // Find & skip qual header
        while(*current++ != '\n')
          BREAK_AT_END;
        BREAK_AT_END;
        if(*current != '+')
          continue;
        cread->seq_e  = current - 1;
        while(*current++ != '\n')
          BREAK_AT_END;
        BREAK_AT_END;
        cread->qual_s = current;
        current      += (cread->seq_e - cread->seq_s);
        BREAK_AT_END;
        cread->qual_e = current - 1;
        if(*current == '\n') {
          nb_reads++;
          break;              // Found a proper sequence/qual pair of lines
        }
      }
    }      
    new_seq->nb_reads = nb_reads;
    if(nb_reads > 0) {
      new_seq->file   = &*current_file;
      current_file->inc();
      rq.enqueue(new_seq);
    }
    if(current >= map_end) {
      current_file->unmap();
      if(++current_file == mapped_files.end()) {
        rq.close();
        break;
      }
      current_file->map();
      current_file->will_need();
      map_base = current_file->base();
      current  = map_base;
      map_end  = current_file->end();
    }
  }
}
