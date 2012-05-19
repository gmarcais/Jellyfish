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

#ifndef __JELLYFISH_MISC_HPP__
#define __JELLYFISH_MISC_HPP__

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <new>
#include <ostream>
#include <utility>

#define bsizeof(v)      (8 * sizeof(v))
typedef uint_fast64_t uint_t;
//#define UINT_C(x)       
#define PRIUINTu PRIuFAST64
#define PRIUINTx PRIxFAST64

inline int leading_zeroes(int x) { return __builtin_clz(x); } // CLK
inline int leading_zeroes(unsigned int x) { return __builtin_clz(x); }
inline int leading_zeroes(unsigned long x) { return __builtin_clzl(x); }
inline int leading_zeroes(unsigned long long x) { return __builtin_clzll(x); }


template <typename T>
unsigned int floorLog2(T n) {
  return sizeof(T) * 8 - 1 - leading_zeroes(n);
}

template<typename T>
uint_t ceilLog2(T n) {
  uint_t r = floorLog2(n);
  return n > (((T)1) << r) ? r + 1 : r;
}

template<typename T>
T div_ceil(T a, T b) {
  T q = a / b;
  return a % b == 0 ? q : q + 1;
}

template<typename T>
uint_t bitsize(T n) {
  return floorLog2(n) + 1;
}

inline uint32_t reverse_bits(uint32_t v) {
  // swap odd and even bits
  v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
  // swap consecutive pairs
  v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
  // swap nibbles ... 
  v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
  // swap bytes
  v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
  // swap 2-byte long pairs
  v = ( v >> 16             ) | ( v               << 16);
  return v;
}

inline uint64_t reverse_bits(uint64_t v) {
  v = ((v >> 1)  & 0x5555555555555555UL) | ((v & 0x5555555555555555UL) << 1);
  v = ((v >> 2)  & 0x3333333333333333UL) | ((v & 0x3333333333333333UL) << 2);
  v = ((v >> 4)  & 0x0F0F0F0F0F0F0F0FUL) | ((v & 0x0F0F0F0F0F0F0F0FUL) << 4);
  v = ((v >> 8)  & 0x00FF00FF00FF00FFUL) | ((v & 0x00FF00FF00FF00FFUL) << 8);
  v = ((v >> 16) & 0x0000FFFF0000FFFFUL) | ((v & 0x0000FFFF0000FFFFUL) << 16);
  v = ( v >> 32                        ) | ( v                         << 32);
  return v;
}

uint64_t bogus_sum(void *data, size_t len);

template <typename T>
size_t bits_to_bytes(T bits) {
  return (size_t)((bits / 8) + (bits % 8 != 0));
}

template <typename T>
union Tptr {
  void *v;
  T    *t;
};
template <typename T>
T *calloc_align(size_t nmemb, size_t alignment) {
  Tptr<T> ptr;
  if(posix_memalign(&ptr.v, alignment, sizeof(T) * nmemb) < 0)
    throw std::bad_alloc();
  return ptr.t;
}

/* Be pedantic about memory access. Any misaligned access will
 * generate a BUS error.
 */
void disabled_misaligned_mem_access();

/* Raison d'etre of this version of mem_copy: It seems we have slow
 * down due to misaligned cache accesses. glibc memcpy does unaligned
 * memory accesses and crashes when they are disabled. This version
 * does only aligned memory access (see above).
 */
template <typename T>
void mem_copy(char *dest,  const char *src, const T &len) {
  // dumb copying char by char
  for(T i = (T)0; i < len; ++i)
    *dest++ = *src++;
}

/* Slice a large number (total) in almost equal parts. return [start,
   end) corresponding to the ith part (0 <= i < number_of_slices)
 */
template<typename T>
std::pair<T,T> slice(T i, T number_of_slices, T total) {
  if(number_of_slices > 1)
    --number_of_slices;
  T slice_size = total / number_of_slices;
  T slice_remain = total % number_of_slices;
  T start = std::min(total, i * slice_size + i * slice_remain / number_of_slices);
  T end = std::min(total, (i + 1) * slice_size + (i + 1) * slice_remain / number_of_slices);
  return std::make_pair(start, end);
}

std::streamoff get_file_size(std::istream& is);
#endif // __MISC_HPP__
