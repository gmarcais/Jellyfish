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

#ifndef __JELLYFISH_SEQ_QUAL_PARSER_HPP__
#define __JELLYFISH_SEQ_QUAL_PARSER_HPP__

#include <iostream>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/file_parser.hpp>
#include <jellyfish/simple_growing_array.hpp>

namespace jellyfish {
  class seq_qual_parser : public file_parser {
  public:
    seq_qual_parser(int fd, const char *path, const char *str, size_t len) :
      file_parser(fd, path, str, len) {}
    virtual ~seq_qual_parser() {}

    struct sequence_t {
      char *start;
      char *end;
    };
    static seq_qual_parser *new_parser(const char *path);

    // parse some input data into the buffer [start, *end). Returns
    // false if there is no more data in the input. **end is an
    // input/output parameter. It points past the end of the buffer
    // when called and should point past the end of the data when
    // returned. The base and its ASCII qual value are one next to
    // another.
    virtual bool parse(char *start, char **end) = 0;

  protected:
  };

  class fastq_seq_qual_parser : public seq_qual_parser {
  public:
    fastq_seq_qual_parser(int fd, const char *path, const char *str, size_t len) :
      seq_qual_parser(fd, path, str, len) {}

    virtual ~fastq_seq_qual_parser() {}
    virtual bool parse(char *start, char **end);

    define_error_class(FastqSeqQualParserError);

  private:
    void copy_qual_values(char *&qual_start, const char *start);
    simple_growing_array<char> _read_buf;
  };
}

#endif
