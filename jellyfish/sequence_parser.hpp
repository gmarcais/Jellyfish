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

#ifndef __JELLYFISH_SEQUENCE_PARSER_HPP__
#define __JELLYFISH_SEQUENCE_PARSER_HPP__

#include <iostream>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/file_parser.hpp>
#include <jellyfish/time.hpp>

namespace jellyfish {
  class sequence_parser : public file_parser {
  protected:
  public:
    sequence_parser(int fd, const char *path, const char *str, size_t len) :
      file_parser(fd, path, str, len) { }
    virtual ~sequence_parser() { }

    struct sequence_t {
      char *start;
      char *end;
    };
    static sequence_parser *new_parser(const char *path);

    // parse some input data into the buffer [start, *end). Returns
    // false if there is no more data in the input. **end is an
    // input/output parameter. It points past the end of the buffer
    // when called and should point past the end of the data when
    // returned.
    virtual bool parse(char *start, char **end) = 0;
  };
  
  class fasta_sequence_parser : public sequence_parser {
  public:
    fasta_sequence_parser(int fd, const char *path, const char *str, size_t len) :
      sequence_parser(fd, path, str, len) {}

    virtual ~fasta_sequence_parser() {}

    virtual bool parse(char *start, char **end);
  };

  class fastq_sequence_parser : public sequence_parser {
    unsigned long seq_len;
  public:
    fastq_sequence_parser(int fd, const char *path, const char *str, 
                          size_t len) :
      sequence_parser(fd, path, str, len), seq_len(0) {}

    virtual ~fastq_sequence_parser() {}
    virtual bool parse(char *start, char **end);

  };
}

#endif
