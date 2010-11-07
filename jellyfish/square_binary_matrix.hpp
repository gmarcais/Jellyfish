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

  uint64_t times_unrolled(uint64_t v) const {
    uint64_t res = 0, *c = columns+(size-1);

    switch(size) {
    case 64: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 63: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 62: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 61: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 60: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 59: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 58: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 57: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 56: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 55: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 54: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 53: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 52: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 51: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 50: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 49: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 48: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 47: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 46: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 45: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 44: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 43: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 42: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 41: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 40: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 39: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 38: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 37: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 36: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 35: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 34: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 33: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 32: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 31: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 30: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 29: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 28: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 27: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 26: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 25: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 24: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 23: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 22: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 21: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 20: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 19: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 18: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 17: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 16: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 15: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 14: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 13: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 12: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 11: res ^= (-(v & 1)) & *c--; v >>= 1;
    case 10: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  9: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  8: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  7: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  6: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  5: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  4: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  3: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  2: res ^= (-(v & 1)) & *c--; v >>= 1;
    case  1: res ^= (-(v & 1)) & *c;
    }
    return res;
  }

#ifdef HAVE_SSE
  uint64_t times_sse(uint64_t v) const {
#define FFs ((uint64_t)-1)
    static const uint64_t smear[8] asm("smear") __attribute__ ((aligned(16),used)) =
      {0, 0, 0, FFs, FFs, 0, FFs, FFs};

    uint64_t *c = columns + (size - 2);
    uint64_t res;
    // How do I make sure that gcc does not use the smear register for something else?
    // Is the & constraint for res enough?

    __asm__ __volatile__ ("pxor %%xmm0, %%xmm0\n"
                          "times_sse_loop%=:\n\t"
                          "movq %2, %0\n\t"
                          "andq $3, %0\n\t"
                          "shlq $4, %0\n\t"
                          "movdqa (%5,%0), %%xmm1\n\t"
                          "pand (%1), %%xmm1\n\t"
                          "pxor %%xmm1, %%xmm0\n\t"
                          "subq $0x10, %1\n\t"
                          "shrq $2, %2\n\t"
                          "movq %2, %0\n\t"
                          "andq $3, %0\n\t"
                          "shlq $4, %0\n\t"
                          "movdqa (%5,%0), %%xmm1\n\t"
                          "pand (%1), %%xmm1\n\t"
                          "pxor %%xmm1, %%xmm0\n\t"
                          "subq $0x10, %1\n\t"
                          "shrq $2, %2\n\t"
                          "jnz times_sse_loop%=\n\t"
                          "movd %%xmm0, %0\n\t"
                          "psrldq $8, %%xmm0\n\t"
                          "movd %%xmm0, %1\n\t"
                          "xorq %1, %0\n\t"
                          : "=&r" (res), "=r" (c), "=r" (v)
                          : "1" (c), "2" (v), "r" (smear)
                          : "xmm0", "xmm1");
    return res;
  }
  inline uint64_t times(uint64_t v) const { return times_sse(v); }
#else
  inline uint64_t times(uint64_t v) const { return times_unrolled(v); }
#endif


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
