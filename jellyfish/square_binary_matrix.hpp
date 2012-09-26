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
#include <algorithm>
#include <assert.h>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>

class SquareBinaryMatrix {
public:
  define_error_class(ErrorAllocation);
  define_error_class(SingularMatrix);
  define_error_class(MismatchingSize);

private:
  uint64_t *columns;
  int       size;

  uint64_t mask() const { return (((uint64_t)1) << size) - 1; }
  uint64_t msb() const { return ((uint64_t)1) << (size - 1); }
  uint64_t *first_alloc(size_t size) {
    uint64_t *res = calloc_align<uint64_t>((size_t)size, (size_t)16);
    if(!res)
      eraise(ErrorAllocation) << "Can't allocate matrix of size '" << size << "'";
    return res;
  }
  void alloc_columns() {
    if(columns) {
      free(columns);
      columns = 0;
    }
    if(size < 0 || size > 64)
      eraise(MismatchingSize) << "Invalid matrix size '" << size << "'";
    columns = first_alloc(size);
  }

public:
  SquareBinaryMatrix() : columns(0), size(0) { }

  explicit SquareBinaryMatrix(int _size) :columns(first_alloc(_size)), size(_size) {
    memset(columns, '\0', sizeof(uint64_t) * _size);
  }
  SquareBinaryMatrix(const SquareBinaryMatrix &rhs) : columns(first_alloc(rhs.get_size())), size(rhs.get_size()) {
    int i;
    
    uint64_t _mask = mask();
    for(i = 0; i < size; i++)
      columns[i] = rhs.columns[i] & _mask;
  }
  SquareBinaryMatrix(const uint64_t *_columns, int _size) : columns(first_alloc(_size)), size(_size) {
    int i;
    uint64_t _mask = mask();

    for(i = 0; i < size; i++)
      columns[i] = _columns[i] & _mask;
  }
  explicit SquareBinaryMatrix(const char *map) : columns(0), size(0) {
    read(map);
  }
  explicit SquareBinaryMatrix(std::istream *is) : columns(0), size(0) {
    load(is);
  }

  ~SquareBinaryMatrix() {
    if(columns)
      free(columns);
  }

  void swap(SquareBinaryMatrix &rhs) {
    std::swap(columns, rhs.columns);
    std::swap(size, rhs.size);
  }

  SquareBinaryMatrix &operator=(SquareBinaryMatrix rhs) {
    this->swap(rhs);
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

  void resize(int ns) { size = ns; alloc_columns(); }
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
  
  inline uint64_t times(uint64_t v) const {
#ifdef HAVE_SSE
    return times_sse(v);
#else
    return times_unrolled(v);
#endif
  }

  SquareBinaryMatrix transpose() const;
  SquareBinaryMatrix operator*(const SquareBinaryMatrix &other) const;

  SquareBinaryMatrix inverse() const;
  int pop_count() const {
    int i, res = 0;
    for(i = 0; i < size; i++)
      res += __builtin_popcountl(columns[i]);
    return res;
  }

  uint64_t xor_sum() const {
    uint64_t sum = 0;
    for(int i = 0; i < size; ++i)
      sum ^= columns[i];
    return sum;
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
