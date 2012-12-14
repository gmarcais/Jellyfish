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

#ifndef __JELLYFISH_FLOATS_HPP__
#define __JELLYFISH_FLOATS_HPP__

#include <stdint.h>
#include <iostream>
#ifdef HALF_FLOATS
#include <jellyfish/half.h>
#endif

namespace jellyfish {
  class Float {
  public:
#ifdef HALF_FLOATS
    typedef uint16_t bits_t;
    typedef half     float_t;
#else
    typedef uint32_t bits_t;
    typedef float    float_t;
#endif

  private:
    union float_int {
      float_t      fv;
      bits_t     iv;
      explicit float_int(float_t v) : fv(v) {}
      explicit float_int(bits_t v) : iv(v) {}
    };
    float_int v;

  public:
    Float() : v(0.0f) {}
    explicit Float(int _v) : v((bits_t)_v) {}
    explicit Float(float_t _v) : v(_v) {}
    explicit Float(bits_t _v) : v(_v) {}

    // static const Float zero;
    // static const Float one;

    const Float operator+(const Float &y) const {
      return Float(v.fv + y.v.fv);
    }
    const Float operator+(const float& y) const {
      return Float(v.fv + y);
    }

    bits_t bits() const { return v.iv; };
    float_t to_float() const { return v.fv; };

    // Should we use the floating point ==?
    bool operator==(Float o) { return v.iv == o.v.iv; }
    friend std::ostream &operator<<(std::ostream &os, const Float &f);
  };
}

#endif
