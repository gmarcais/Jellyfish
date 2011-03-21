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

#ifndef __JELLYFISH_FASTQ_SEQUENCE_PARSER_HPP__
#define __JELLYFISH_FASTQ_SEQUENCE_PARSER_HPP__

#include <jellyfish/file_parser.hpp>

namespace jellyfish {
  class fastq_sequence_parser : public file_parser {
    unsigned long seq_len;

  public:
    fastq_sequence_parser(int fd, const char *path, const char *str, size_t len) :
      file_parser(fd, path, str, len), seq_len(0) {}
    ~fastq_sequence_parser() {}

    virtual bool parse(char *start, char **end);
  };
}
#endif
