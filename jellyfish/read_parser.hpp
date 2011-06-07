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

#ifndef __JELLYFISH__READ_PARSER_HPP__
#define __JELLYFISH__READ_PARSER_HPP__

#include <jellyfish/file_parser.hpp>

namespace jellyfish {
  class read_parser : public file_parser {
  public:
    struct read_t {
      const char *seq_s, *seq_e, *header, *qual_s, *qual_e;
      size_t      hlen;
    };
    struct reads_t {
      static const int max_nb_reads = 100;
      int              nb_reads;
      read_t           reads[max_nb_reads];
    };

    read_parser(int fd, const char *path, const char *str, size_t len) :
      file_parser(fd, path, str, len) {}

    virtual ~read_parser() {}

    static read_parser *new_parser(const char *path);
    virtual bool next_reads(reads_t *rs) = 0;
  };

  class fasta_read_parser : public read_parser {
  public:
    fasta_read_parser(int fd, const char *path, const char *str, size_t len) :
      read_parser(fd, path, str, len) {}

    virtual ~fasta_read_parser() {}
    virtual bool next_reads(reads_t *rs);
  };

  class fastq_read_parser : public read_parser {
  public:
    fastq_read_parser(int fd, const char *path, const char *str, size_t len) :
      read_parser(fd, path, str, len) {}
    virtual ~fastq_read_parser() {}
    virtual bool next_reads(reads_t *rs);
  };
}
#endif
