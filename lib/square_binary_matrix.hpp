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
  SquareBinaryMatrix() : columns(NULL), size(-1) { }
  uint64_t mask() const { return (((uint64_t)1) << size) - 1; }
  uint64_t msb() const { return ((uint64_t)1) << (size - 1); }

public:
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
