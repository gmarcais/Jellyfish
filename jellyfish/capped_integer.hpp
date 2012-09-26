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

#ifndef __JELLYFISH_CAPPED_INTEGER_HPP__
#define __JELLYFISH_CAPPED_INTEGER_HPP__

#include <iostream>

namespace jellyfish {
  template<typename T> class capped_integer;
  template<typename T>
  std::ostream &operator<<(std::ostream &os, const capped_integer<T> &i);

  template<typename T>
  class capped_integer {
    T x;

  public:
    typedef T bits_t;
    static const T cap = (T)-1;

    capped_integer() : x(0) {}
    explicit capped_integer(bits_t _x) : x(_x) {}

    static const capped_integer zero;
    static const capped_integer one;
    
    const capped_integer operator+(const capped_integer y) const {
      return capped_integer((y.x > ~x) ? cap : y.x + x);
    }
    const capped_integer operator+(const T& y) const {
      return capped_integer((y > ~x) ? cap : y + x);
    }

    bits_t bits() const { return x; }
    float to_float() const { return (float)x; }
   
    bool operator==(const capped_integer &o) { return x == o.x; }
    bool operator!() const { return x == 0; }
    
    friend std::ostream &operator<< <> (std::ostream &os, 
                                        const capped_integer<T> &i);
  };

  template<typename T>
  std::ostream &operator<<(std::ostream &os, const capped_integer<T> &i) {
    return os << i.x;
  }
}

#endif
