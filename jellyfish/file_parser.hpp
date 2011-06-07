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

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>

namespace jellyfish {
  class file_parser {
    int                  _fd;
    char                 _base, _pbase;
    static const size_t  _buff_size = 1024 * 1024;
    char                *_buffer;
    const char          *_end_buffer;
    const char          *_data;
    const char          *_end_data;
    size_t               _size;
    bool                 _is_mmapped;

    static bool          _do_mmap;
    static bool          _force_mmap;

  protected:
    define_error_class(FileParserError);
    int sbumpc();
    int speekc();

  public:
    // [str, str+len) is content on initial buffer
    file_parser(int fd, const char *path, const char *str, size_t len,
                char pbase = '\n');
    ~file_parser();

    static bool do_mmap();
    static bool do_mmap(bool new_value);
    // throw an error if mmap fails
    static bool force_mmap();
    static bool force_mmap(bool new_value);
    static int file_peek(const char *path, char *peek);

    // current base and previous base
    char base() const { return _base; }
    char pbase() const { return _pbase; }
    bool eof() const { return _base == EOF; }

    // ptr to base. Valid only for mmaped files
    const char *ptr() const { return _data; }
    const char *base_ptr() const { return _data - 1; }
    const char *pbase_ptr() const { return _data; }

  private:
    int read_next_buffer();
  };
}

#endif
