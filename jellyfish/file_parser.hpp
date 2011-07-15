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
#include <jellyfish/time.hpp>

namespace jellyfish {
  class file_parser {
    int         _fd;
    int         _base, _pbase;
    char       *_buffer;
    const char *_end_buffer;
    const char *_data;
    const char *_end_data;
    size_t      _size;
    bool        _is_mmapped;

    static bool _do_mmap;
    static bool _force_mmap;

  protected:
    define_error_class(FileParserError);
    // Get next character in "stream"
    inline int sbumpc() {
      _pbase = _base;
      if(_data >= _end_data)
        if(!read_next_buffer())
          return _base = _eof;
      return (_base = *_data++);
    }
    int speekc() {
      if(_data >= _end_data)
        if(!read_next_buffer())
          return _eof;
      return *_data;
    }


  public:
    static const size_t _buff_size = 1024 * 1024;
    static const int    _eof       = -1;

    // [str, str+len) is content on initial buffer
    file_parser(int fd, const char *path, const char *str, size_t len,
                char pbase = '\n');
    ~file_parser();

    static bool do_mmap() { return _do_mmap; };
    static bool do_mmap(bool new_value) { bool oval = _do_mmap; _do_mmap = new_value; return oval; }
    // throw an error if mmap fails
    static bool force_mmap();
    static bool force_mmap(bool new_value);
    static int file_peek(const char *path, char *peek);

    // current base and previous base
    int base() const { return _base; }
    int pbase() const { return _pbase; }
    bool eof() const { return _base == _eof; }

    // ptr to base. Valid only for mmaped files
    const char *ptr() const { return _data; }
    const char *base_ptr() const { return _data - 1; }
    const char *pbase_ptr() const { return _data; }

  private:
    // Buffers next chunk of data. Returns _eof if at end of file or next character
    bool read_next_buffer();
  };
}

#endif
