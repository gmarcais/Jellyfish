#ifndef __SQUARE_BINARY_MATRIX_HPP__
#define __SQUARE_BINARY_MATRIX_HPP__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <exception>
#include <assert.h>
#include "misc.hpp"

#ifdef SSE
typedef long long int i64_t;
typedef i64_t v2di __attribute__ ((vector_size(16)));
union vector2di {
  i64_t i[2];
  v2di v;
};
#define FFs ((uint64_t)-1)
static const vector2di smear1[4] = {
  {{0, 0}}, {{0, FFs}}, {{FFs, 0}}, {{FFs, FFs}}
};
static const v2di smear[4] = { smear1[0].v, smear1[1].v, smear1[2].v, smear1[3].v };
static const vector2di zero1 = { {0, 0} };
static const v2di zero = zero1.v;
#endif

class SquareBinaryMatrix {
  uint64_t *columns;
  int       size;

private:
  uint64_t mask() const { return (((uint64_t)1) << size) - 1; }
  uint64_t msb() const { return ((uint64_t)1) << (size - 1); }

public:
  SquareBinaryMatrix() : columns(NULL), size(-1) { }

  SquareBinaryMatrix(int _size) {
    columns = new uint64_t[_size];
    size = _size;
    memset(columns, '\0', sizeof(uint64_t) * _size);
  }
  SquareBinaryMatrix(const SquareBinaryMatrix &rhs) {
    int i;
    columns = new uint64_t[rhs.get_size()];
    size = rhs.get_size();
    uint64_t _mask = mask();
    for(i = 0; i < size; i++)
      columns[i] = rhs.columns[i] & _mask;
  }
  SquareBinaryMatrix(const uint64_t *_columns, int _size) {
    int i;
    size = _size;
    uint64_t _mask = mask();
    columns = new uint64_t[_size];

    for(i = 0; i < size; i++)
      columns[i] = _columns[i] & _mask;
  }
  ~SquareBinaryMatrix() {
    if(columns)
      delete[] columns;
  }

  class SingularMatrix : public std::exception {
    virtual const char* what() const throw() {
      return "Matrix is singular";
    }
  };
  class MismatchingSize : public std::exception {
    virtual const char  *what() const throw() {
      return "Matrix sizes do not match";
    }
  };


  SquareBinaryMatrix &operator=(const SquareBinaryMatrix &rhs) {
    int i;
    if(this == &rhs)
      return *this;
    if(columns)
      delete[] columns;
    columns = new uint64_t[rhs.get_size()];
    size = rhs.get_size();
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

#ifdef SSE
  uint64_t times_sse(uint64_t v) const {
    v2di *c = (v2di *)(columns + (size - 1));
    v2di res = zero;
    vector2di res1;
    int i;
    v2di tmp;
        
    // Only works if size is even
    for(i = size / 2; i >= 0; i--, c--) {
      tmp = __builtin_ia32_lddqu((const char *)c);
      tmp = __builtin_ia32_pand128(smear[v & 0x3], tmp);
      res = __builtin_ia32_pxor128(res, tmp);
      v >>= 2;
    }

    res1.v = res;
    return res1.i[0] ^ res1.i[1];
  }
#endif

  inline uint64_t times(uint64_t v) const { return times_loop(v); }

  SquareBinaryMatrix transpose() const;
  SquareBinaryMatrix operator*(const SquareBinaryMatrix &other) const;

  SquareBinaryMatrix inverse() const;
  int pop_count() const {
    int i, res = 0;
    for(i = 0; i < size; i++)
      res += __builtin_popcountl(columns[i]);
    return res;
  }

  void print(std::ostream &os) const;
  std::string str() const;
  void dump(std::ostream &os) const;
  void load(std::istream &is);
  size_t read(const char *map);
  void print_vector(std::ostream &os, uint64_t v, bool vertical = false) const;
  std::string str_vector(uint64_t v, bool vertical = false) const;
};

#endif // __SQUARE_BINARY_MATRIX_HPP__
