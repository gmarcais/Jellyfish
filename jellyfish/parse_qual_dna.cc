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

#include <jellyfish/parse_qual_dna.hpp>

namespace jellyfish {
  parse_qual_dna::parse_qual_dna(int nb_files, char *argv[], uint_t _mer_len,
                                 unsigned int nb_buffers, size_t _buffer_size,
                                 const char _qs, const char _min_q) :
    rq(nb_buffers), wq(nb_buffers), mer_len(_mer_len), 
    reader(0), buffer_size(_buffer_size), files(argv, argv + nb_files),
    current_file(files.begin()), have_seam(false), quality_start(_qs),
    min_q(_min_q)
  {
    buffer_data = new char[nb_buffers * buffer_size];
    buffers     = new seq[nb_buffers];
    seam        = new char[2*mer_len];
    
    for(unsigned int i = 0; i < nb_buffers; i++) {
      buffers[i].start = buffer_data + i * _buffer_size;
      buffers[i].end   = buffers[i].start;
      wq.enqueue(&buffers[i]);
    }

    fparser = jellyfish::file_parser::new_file_parser_seq_qual(*current_file);
  }

  bool parse_qual_dna::thread::next_sequence() {
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

  void parse_qual_dna::_read_sequence() {
    seq *new_seq = 0;
  
    while(true) {
      if(!new_seq) {
        new_seq = wq.dequeue();
        if(!new_seq)
          break;
      }
      new_seq->end = new_seq->start + buffer_size;
      char *start = new_seq->start;
      if(have_seam) {
        have_seam = false;
        memcpy(start, seam, 2 * (mer_len - 1));
        start += 2 * (mer_len - 1);
      }
      bool input_eof = !fparser->parse(start, &new_seq->end);
      if(new_seq->end > new_seq->start + 2 * mer_len) {
        have_seam = true;
        memcpy(seam, new_seq->end - 2 * (mer_len - 1), 2 * (mer_len - 1));
        rq.enqueue(new_seq);
        new_seq = 0;
      }
      if(input_eof) {
        delete fparser;
        have_seam = false;
        if(++current_file == files.end()) {
          rq.close();
          break;
        }
        fparser = jellyfish::file_parser::new_file_parser_seq_qual(*current_file);
      }
    }
  }

  const uint_t parse_qual_dna::codes[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };
}
