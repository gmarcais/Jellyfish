#ifndef __MISC_HPP__
#define __MISC_HPP__

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>


#define bsizeof(v)      (8 * sizeof(v))
#define PRINTVAR(v) {std::cout << __LINE__ << " " #v ": " << v << std::endl; }
typedef uint_fast16_t uint_t;
#define UINT_C(x)       UINT
#define PRIUINTu PRIuMAX
#define PRIUINTx PRIxMAX

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

/* Like Perl's die function
 */
void die(const char *msg, ...) __attribute__ ((noreturn, format(printf, 1, 2)));

#endif // __MISC_HPP__
