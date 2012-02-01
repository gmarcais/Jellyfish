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

#include <jellyfish/atomic_gcc.hpp>

class counter_t {
  volatile uint64_t     count;

public:
  counter_t() : count(0) {}

  inline uint64_t operator++(int) {
    return atomic::gcc::fetch_add(&count, (uint64_t)1);
  }
  inline uint64_t inc(uint64_t x) {
    return atomic::gcc::fetch_add(&count, x);
  }
  inline uint64_t get() const { return count; }

  class block {
    counter_t *c;
    uint64_t   bs;
    uint64_t   base, i;

    //    friend counter_t;
  public:
    block(counter_t *_c, uint64_t _bs) : c(_c), bs(_bs), base(0), i(bs) {}

  public:
    inline uint64_t operator++(int) {
      if(i >= bs) {
        i = 0;
        base = c->inc(bs);
      }
      return base + i++;
    }
  };
  block get_block(uint64_t bs = 100) { return block(this, bs); }
};
