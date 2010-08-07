#include <misc.hpp>
#include <stdint.h>

class divisor64 {
  uint64_t d, p, m;
public:
  // This initialization works provided that 0<=_d<2**32
  divisor64(uint64_t _d) : d(_d) {
    uint64_t q = (1UL << 63) / d;
    uint64_t r = (1UL << 63) % d;
    p = ceilLog2(d) - 1;
    m = q * (1UL << (p + 2)) + div_ceil(r * (1UL << (p + 2)), d);
  }

  divisor64() : d(0), p(0), m(0) {}
  
  uint64_t divide(uint64_t x) const {
    __asm__("movq %0, %%rax\n\t"
            "mulq %1\n\t"
            "subq %%rdx, %0\n\t"
            "shrq %0\n\t"
            "addq %%rdx, %0\n\t"
            "shrq %%cl, %0\n\t"
            : "=r" (x)
            : "r" (m), "c" (p), "0" (x)
            : "rax", "rdx");
    return x;
  }

  void division(uint64_t x, uint64_t &q, uint64_t &r) const {
    q = divide(x);
    r = x - q * d;
  }
};
