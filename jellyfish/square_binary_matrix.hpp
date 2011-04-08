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

#ifndef __JELLYFISH_SQUARE_BINARY_MATRIX_HPP__
#define __JELLYFISH_SQUARE_BINARY_MATRIX_HPP__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <exception>
#include <assert.h>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>

class SquareBinaryMatrix {
  uint64_t *columns;
  int       size;

private:
  uint64_t mask() const { return (((uint64_t)1) << size) - 1; }
  uint64_t msb() const { return ((uint64_t)1) << (size - 1); }
  void alloc_columns() {
    if(columns)
      free(columns);
    columns = calloc_align<uint64_t>((size_t)size, (size_t)16);
  }

public:
  SquareBinaryMatrix() : columns(NULL), size(-1) { }

  SquareBinaryMatrix(int _size) :columns(NULL), size(_size) {
    alloc_columns();
    memset(columns, '\0', sizeof(uint64_t) * _size);
  }
  SquareBinaryMatrix(const SquareBinaryMatrix &rhs) : columns(NULL) {
    int i;
    
    size = rhs.get_size();
    alloc_columns();
    uint64_t _mask = mask();
    for(i = 0; i < size; i++)
      columns[i] = rhs.columns[i] & _mask;
  }
  SquareBinaryMatrix(const uint64_t *_columns, int _size) : columns(NULL), size(_size) {
    int i;
    uint64_t _mask = mask();
    alloc_columns();

    for(i = 0; i < size; i++)
      columns[i] = _columns[i] & _mask;
  }
  SquareBinaryMatrix(const char *map) : columns(NULL), size(0) {
    read(map);
  }
  SquareBinaryMatrix(std::istream *is) : columns(NULL), size(0) {
    load(is);
  }

  ~SquareBinaryMatrix() {
    if(columns)
      free(columns);
  }

  define_error_class(SingularMatrix);
  define_error_class(MismatchingSize);

  SquareBinaryMatrix &operator=(const SquareBinaryMatrix &rhs) {
    int i;
    if(this == &rhs)
      return *this;
    size = rhs.get_size();
    alloc_columns();
    uint64_t _mask = mask();
    for(i = 0; i < size; i++)
      columns[i] = rhs.columns[i] & _mask;

    return *this;
  }

  void init_random();

  SquareBinaryMatrix init_random_inverse();

  void init_identity() {
    uint64_t v = msb();
    int i;
    for(i = 0; v; i++, v >>= 1)
      columns[i] = v;
  }

  bool is_identity() const {
    uint64_t v = msb();
    int i;

    for(i = 0; i < size; i++, v >>= 1) {
      if(columns[i] != v)
        return false;
    }
    return true;
  }

  bool is_zero() const {
    int i;
    for(i = 0; i < size; i++)
      if(columns[i])
        return false;
    return true;
  }

  int get_size() const { return size; }

  bool operator==(const SquareBinaryMatrix &other) const {
    int i;
    if(size != other.get_size())
      return false;
    for(i = 0; i < size; i++)
      if(columns[i] != other.columns[i])
        return false;

    return true;
  }

  bool operator!=(const SquareBinaryMatrix &other) const {
    return !(*this == other);
  }
  
  uint64_t & operator[](int i) {
    return columns[i];
  }

  uint64_t operator[](int i) const {
    return columns[i];
  }

  uint64_t times_loop(uint64_t v) const {
    uint64_t res = 0, *c = columns+(size-1);
    
    for ( ; v; v >>= 1)
      res ^= (-(v & 1)) & *c--;
    return res;
  }

  uint64_t times_unrolled(uint64_t v) const;
  uint64_t times_sse(uint64_t v) const;
  uint64_t times(uint64_t v) const;

  SquareBinaryMatrix transpose() const;
  SquareBinaryMatrix operator*(const SquareBinaryMatrix &other) const;

  SquareBinaryMatrix inverse() const;
  int pop_count() const {
    int i, res = 0;
    for(i = 0; i < size; i++)
      res += __builtin_popcountl(columns[i]);
    return res;
  }

  void print(std::ostream *os) const;
  std::string str() const;
  void dump(std::ostream *os) const;
  void load(std::istream *is);
  size_t read(const char *map);
  size_t dump_size() { return sizeof(size) + sizeof(uint64_t) * size; }
  void print_vector(std::ostream *os, uint64_t v, bool vertical = false) const;
  std::string str_vector(uint64_t v, bool vertical = false) const;
};

#endif // __SQUARE_BINARY_MATRIX_HPP__
