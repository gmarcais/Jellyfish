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

#ifndef __JELLYFISH_FILE_PARSER_HPP__
#define __JELLYFISH_FILE_PARSER_HPP__

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <jellyfish/misc.hpp>

namespace jellyfish {
  class file_parser {
    int _fd;
    static const size_t  _buff_size = 8192;
    char                *_buffer;
    const char          *_end_buffer;
    const char          *_data;
    const char          *_end_data;
    size_t               _size;
    bool                 _is_mmapped;

  protected:
    define_error_class(FileParserError);
    int sbumpc();

  public:
    file_parser(int fd, const char *path);
    ~file_parser();
    static file_parser *new_file_parser(const char *path);

    // parse some input data into the buffer [start, *end). Returns
    // false if there is no more data in the input. **end is an
    // input/output parameter. It points past the end of the buffer
    // when called and should point past the end of the data when
    // returned.
    virtual bool parse(char *start, char **end) = 0;
  };
}

#include <fasta_parser.hpp>
// #include <fastq_sequence_parser.hpp>


#endif
