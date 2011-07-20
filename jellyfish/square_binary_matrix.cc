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

#include <config.h>
#include <jellyfish/square_binary_matrix.hpp>
#include <iostream>
#include <sstream>

void SquareBinaryMatrix::init_random() {
  uint64_t _mask = mask();
  int i, j;
  int lrm = floorLog2((unsigned long)RAND_MAX);
  for(i = 0; i < size; i++) {
    for(j = 0; j < size; j += lrm)
      columns[i] ^= (uint64_t)random() << j;
    columns[i] &= _mask;
  }
}

SquareBinaryMatrix SquareBinaryMatrix::init_random_inverse() {
  while(true) {
    init_random();
    try {
      return inverse();
    } catch(SquareBinaryMatrix::SingularMatrix &e) { }
  }
}

SquareBinaryMatrix SquareBinaryMatrix::transpose() const {
  SquareBinaryMatrix res(size);
  uint64_t *a = res.columns, *b;
  int i, j;

  for(i = size - 1; i >= 0; i--, a++) 
    for(j = size - 1, b = columns; j >= 0; j--, b++)
      *a |= ((*b >> i) & 0x1) << j;

  return res;
}

SquareBinaryMatrix SquareBinaryMatrix::operator*(const SquareBinaryMatrix &other) const {
  int i;
  SquareBinaryMatrix res(size);

  if(size != other.get_size()) 
    eraise(MismatchingSize) << "Multiplication operator dimension mismatch:" 
                            << size << "x" << size << " != " 
                            << other.get_size() << "x" << other.get_size();
  
  for(i = 0; i < size; i++) {
    res[i] = this->times(other[i]);
  }

  return res;
}

SquareBinaryMatrix SquareBinaryMatrix::inverse() const {
  SquareBinaryMatrix pivot(*this);
  SquareBinaryMatrix res(size);
  int i, j;

  res.init_identity();

  // forward elimination
  for(i = 0; i < size; i++) {
    if(!((pivot.columns[i] >> (size - i - 1)) & (uint64_t)0x1)) {
      for(j = i + 1; j < size; j++)
        if((pivot.columns[j] >> (size - i - 1)) & (uint64_t)0x1)
          break;
      if(j >= size)
	eraise(SingularMatrix) << "Matrix is singular";
      pivot.columns[i] ^= pivot.columns[j];
      res.columns[i] ^= res.columns[j];
    }

    for(j = i+1; j < size; j++) {
      if((pivot.columns[j] >> (size - i - 1)) & (uint64_t)0x1) {
	pivot.columns[j] ^= pivot.columns[i];
	res.columns[j] ^= res.columns[i];
      }
    }
  }

  // backward elimination
  for(i = size - 1; i > 0; i--) {
    for(j = i - 1; j >= 0; j--) {
      if((pivot.columns[j] >> (size - i - 1)) & (uint64_t)0x1) {
	pivot.columns[j] ^= pivot.columns[i];
	res.columns[j] ^= res.columns[i];
      }
    }
  }
  return res;
}

void SquareBinaryMatrix::print(std::ostream *os) const {
  int i, j;
  uint64_t a, *v;

  *os << size << "x" << size << std::endl;
  for(i = 0, a = msb(); i < size; i++, a >>= 1) {
    for(j = 0, v = columns; j < size; j++, v++)
      *os << ((*v & a) ? 1 : 0);
    *os << std::endl;
  }
}

std::string SquareBinaryMatrix::str() const {
  std::ostringstream os;
  print(&os);
  return os.str();
}

void SquareBinaryMatrix::dump(std::ostream *os) const {
  os->write((char *)&size, sizeof(size));
  os->write((char *)columns, sizeof(uint64_t) * size);
}

void SquareBinaryMatrix::load(std::istream *is) {
  is->read((char *)&size, sizeof(size));
  alloc_columns();
  is->read((char *)columns, sizeof(uint64_t) * size);
}

size_t SquareBinaryMatrix::read(const char *map) {
  int nsize = 0;
  memcpy(&nsize, map, sizeof(nsize));
  if(nsize <= 0 || nsize > 64)
    eraise(MismatchingSize) << "Invalid matrix size '" << nsize << "'. Must be between 1 and 64";

  size = nsize;
  alloc_columns();
  memcpy(columns, map + sizeof(size), sizeof(uint64_t) * size);
  return sizeof(size) + sizeof(uint64_t) * size;
}

void SquareBinaryMatrix::print_vector(std::ostream *os, uint64_t v, bool vertical) const {
  uint64_t a;
  for(a = msb(); a; a >>= 1)
    *os << ((v & a) ? 1 : 0) << (vertical ? "\n" : "");
}

std::string SquareBinaryMatrix::str_vector(uint64_t v, bool vertical) const {
  std::ostringstream os;
  print_vector(&os, v, vertical);
  return os.str();
}

#ifdef HAVE_SSE
uint64_t SquareBinaryMatrix::times_sse(uint64_t v) const {
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

uint64_t SquareBinaryMatrix::times(uint64_t v) const {
  return times_sse(v);
}

#else

uint64_t SquareBinaryMatrix::times_sse(uint64_t v) const { return 0; }
uint64_t SquareBinaryMatrix::times(uint64_t v) const {
  return times_unrolled(v);
}

#endif

uint64_t SquareBinaryMatrix::times_unrolled(uint64_t v) const {
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

