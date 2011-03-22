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

#ifndef __JELLYFISH_FASTQ_SEQ_QUAL_PARSER_HPP__
#define __JELLYFISH_FASTQ_SEQ_QUAL_PARSER_HPP__

#include <jellyfish/file_parser.hpp>

// Simple growing array. Never shrinks.
template<typename T>
class simple_growing_array {
  size_t  _capacity;
  size_t  _size;
  T      *_data;
public:
  simple_growing_array(size_t capacity = 100) : 
    _capacity(capacity / 2), _size(0), _data(0) { resize(); }

  ~simple_growing_array() {
    free(_data);
  }

  void push_back(const T &x) {
    if(_size >= _capacity)
      resize();
    _data[_size++] = x;
  }

  void reset() { _size = 0; }

  size_t size() const { return _size; }
  bool empty() const { return _size == 0; }
  const T * begin() const { return _data; }
  const T * end() const { return _data + _size; }
    
private:
  define_error_class(SimpleGrowingArrayError);
  void resize() {
    _capacity *= 2;
    _data = (T *)realloc(_data, sizeof(T) * _capacity);
    if(_data == 0)
      raise(SimpleGrowingArrayError) << "Out of memory" << err::no;
  }
};

namespace jellyfish {
  class fastq_seq_qual_parser : public file_parser {
    simple_growing_array<char> _read_buf;

  public:
    define_error_class(FastqSeqQualParserError);
    fastq_seq_qual_parser(int fd, const char *path, const char *str, size_t len) :
      file_parser(fd, path, str, len) {}
    ~fastq_seq_qual_parser() {}

    virtual bool parse(char *start, char **end);

  private:
    void copy_qual_values(char *&qual_start, const char *start);
  };
}
#endif
