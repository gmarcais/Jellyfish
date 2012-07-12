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

#ifndef __DIVISOR_HPP__
#define __DIVISOR_HPP__

#include <stdint.h>
#include <config.h>
#include <jellyfish/misc.hpp>

class divisor64 {
  uint64_t d;
#ifdef HAVE_INT128
  uint64_t p, m;
#endif

public:
  // This initialization works provided that 0<=_d<2**32
  divisor64(uint64_t _d) : d(_d) {
#ifdef HAVE_INT128
    uint64_t q = (1UL << 63) / d;
    uint64_t r = (1UL << 63) % d;
    p = ceilLog2(d) - 1;
    m = q * (1UL << (p + 2)) + div_ceil(r * (1UL << (p + 2)), d);
#endif
  }

  //  divisor64() : d(0), p(0), m(0) {}
  
  uint64_t divide(uint64_t x) const {
#ifdef HAVE_INT128
    uint64_t y = ((__int128)x * (__int128)m) >> 64;
    return (((x - y) >> 1) + y) >> p;
#else
    return x / d;
#endif
  }

  void division(uint64_t x, uint64_t &q, uint64_t &r) const {
    q = divide(x);
    r = x - q * d;
  }
};

#endif /* __DIVISOR_HPP__ */
