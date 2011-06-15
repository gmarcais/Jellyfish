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

#include <jellyfish/read_parser.hpp>
#include <jellyfish/dbg.hpp>

jellyfish::read_parser *
jellyfish::read_parser::new_parser(const char *path) {
  int fd;
  char peek;
  fd = file_peek(path, &peek);

  switch(peek) {
  case '>': return new fasta_read_parser(fd, path, &peek, 1);
  case '@': return new fastq_read_parser(fd, path, &peek, 1);
  default:
    eraise(FileParserError) << "'" << path << "'" 
                            << "Invalid input file. Expected fasta or fastq format";
  }
  return 0;
}

#define BREAK_AT_END if(eof()) break;
namespace jellyfish {
  bool fasta_read_parser::next_reads(read_parser::reads_t *rs) {
    for(rs->nb_reads = 0; 
        rs->nb_reads < read_parser::reads_t::max_nb_reads && !eof();
        ++rs->nb_reads) {
      read_t *r = &rs->reads[rs->nb_reads];
      r->qual_s = r->qual_e = 0;
      while(sbumpc() != '>')
        BREAK_AT_END;
      BREAK_AT_END;
      r->header = ptr();
      while(sbumpc() != '\n')
        BREAK_AT_END;
      BREAK_AT_END;
      r->hlen  = ptr() - r->header - 1;
      r->seq_s = ptr();
      // find end of sequence
      while(!eof())
        if(sbumpc() == '\n')
          if(speekc() == '>')
            break;
      r->seq_e = ptr();
    }
    return !eof();
  }


  bool fastq_read_parser::next_reads(read_parser::reads_t *rs) {
    for(rs->nb_reads = 0; 
        rs->nb_reads < read_parser::reads_t::max_nb_reads && !eof();
        ++rs->nb_reads) {
      read_t *r = &rs->reads[rs->nb_reads];
      while(sbumpc() != '@')
        BREAK_AT_END;
      BREAK_AT_END;
      r->header = ptr();
      while(sbumpc() != '\n')
        BREAK_AT_END;
      BREAK_AT_END;
      r->hlen  = ptr() - r->header - 1;
      r->seq_s = ptr();
      // find end of sequence
      size_t nb_bases = 0;
      while(!eof()) {
        if(sbumpc() == '\n') {
          if(speekc() == '+')
            break;
        } else {
          ++nb_bases;
        }
      }
      r->seq_e = ptr();

      // Skip qual header
      while(sbumpc() != '\n')
        BREAK_AT_END;
      BREAK_AT_END;
      r->qual_s = ptr();

      // Skip qual values
      while(!eof() && nb_bases > 0)
        if(sbumpc() != '\n')
          --nb_bases;
      if(nb_bases == 0)
        r->qual_e = ptr();
      else
        BREAK_AT_END;
    }
    return !eof();
  } 
}

