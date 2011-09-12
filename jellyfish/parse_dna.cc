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

#include <jellyfish/parse_dna.hpp>
#include <jellyfish/time.hpp>

namespace jellyfish {
  void parse_dna::fill() {
    bucket_t *new_seq = 0;

    while(true) {
      if(!new_seq) {
        new_seq = write_next();
        if(!new_seq)
          break;
      }
      new_seq->end = new_seq->start + buffer_size;
      char *start = new_seq->start;
      if(have_seam) {
        have_seam = false;
        //        mem_copy(start, seam, mer_len - 1);
        memcpy(start, seam, mer_len - 1);
        start += mer_len - 1;
      }
      
      bool input_eof = !fparser->parse(start, &new_seq->end);

      if(new_seq->end > new_seq->start + mer_len) {
        have_seam = true;
        //        mem_copy(seam, new_seq->end - mer_len + 1, mer_len - 1);
        memcpy(seam, new_seq->end - mer_len + 1, mer_len - 1);
        write_release(new_seq);
        new_seq = 0;
      }
      if(input_eof) {
        delete fparser;
        have_seam = false;
        if(++current_file == files.end()) {
          close();
          break;
        }
        fparser = sequence_parser::new_parser(*current_file);
      }
    }
  }

  parse_dna::parse_dna(const std::vector<const char *> &_files,
                       uint_t _mer_len,
                       unsigned int nb_buffers, size_t _buffer_size) :
    double_fifo_input<sequence_parser::sequence_t>(nb_buffers), mer_len(_mer_len), 
    buffer_size(allocators::mmap::round_to_page(_buffer_size)),
    files(_files), current_file(files.begin()),
    have_seam(false), buffer_data(buffer_size * nb_buffers), canonical(false)
  {
    seam        = new char[mer_len];
    memset(seam, 'A', mer_len);

    unsigned long i = 0;
    for(bucket_iterator it = bucket_begin();
        it != bucket_end(); ++it, ++i) {
      it->end = it->start = (char *)buffer_data.get_ptr() + i * buffer_size;
    }
    assert(i == nb_buffers);

    fparser = sequence_parser::new_parser(*current_file);
  }

  const uint_t parse_dna::codes[256] = {
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -2, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -1, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3,  0, -1,  1, -1, -3, -3,  2, -1, -3, -3, -1, -3, -1, -1, -3, 
    -3, -3, -1, -1,  3, -3, -1, -1, -1, -1, -3, -3, -3, -3, -3, -3, 
    -3,  0, -1,  1, -1, -3, -3,  2, -1, -3, -3, -1, -3, -1, -1, -3, 
    -3, -3, -1, -1,  3, -3, -1, -1, -1, -1, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3
  };
}
