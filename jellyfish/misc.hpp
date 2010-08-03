#ifndef __MISC_HPP__
#define __MISC_HPP__

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <exception>
#include <new>

#define bsizeof(v)      (8 * sizeof(v))
#define PRINTVAR(v) {std::cout << __LINE__ << " " #v ": " << v << std::endl; }
typedef uint_fast16_t uint_t;
//#define UINT_C(x)       
#define PRIUINTu PRIuFAST16
#define PRIUINTx PRIxFAST16

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

/* Like Perl's die function
 */
void die(const char *msg, ...) __attribute__ ((noreturn, format(printf, 1, 2)));

class StandardError : public std::exception {
  char msg[4096];
public:
  // __attribute__ ((format(printf, 1, 2)))
  StandardError(const char *fmt, ...) {
    va_list ap;
    
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg) - 1, fmt, ap);
    msg[sizeof(msg)-1] = '\0';
    va_end(ap);
  }

  // find proper __attribute__ ((format(printf, 1, 2)))
  StandardError(int errnum, const char *fmt, ...) {
    va_list ap;
    int len;
    
    va_start(ap, fmt);
    len = vsnprintf(msg, sizeof(msg) - 1, fmt, ap);
    if(len < (int)(sizeof(msg)-1))
      snprintf(msg + len, sizeof(msg) - len - 1,
               ": %s", strerror(errnum));
    msg[sizeof(msg)-1] = '\0';
    va_end(ap);
  }

  virtual const char* what() const throw() {
    return msg;
  }
};

/**
 * Convert a bit-packed key to a char* string
 **/
inline void tostring(uint64_t key, unsigned int rklen, char * out) {
  static const char table[4] = { 'A', 'C', 'G', 'T' };

  for(unsigned int i = 0 ; i < rklen; i++) {
    out[rklen-1-i] = table[key & UINT64_C(0x3)];
    key >>= 2;
  }
  out[rklen] = '\0';
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
#endif // __MISC_HPP__
