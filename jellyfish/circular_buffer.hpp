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

#ifndef __JELLYFISH_CIRCULAR_BUFFER_HPP__
#define __JELLYFISH_CIRCULAR_BUFFER_HPP__

#include <iostream>

namespace jellyfish {
  template<typename T>
  class circular_buffer {
    const int  size;
    T         *buffer, *end;
    T         *start;

  public:
    explicit circular_buffer(int _size) : size(_size) {
      buffer = new T[size];
      end    = buffer + size;
      start  = buffer;
    }

    ~circular_buffer() {
      delete [] buffer;
    }

    void append(const T v) {
      //      std::cerr << "append buffer " << (void *)buffer << " end " << (void *)end << " start " << (void *)start << " val " << v << "\n";
      *start++ = v;
      
      if(start == end)
        start = buffer;
    }

    template<typename U>
    T op(U o) const {
      T *c = start;
      T acc = *c++;
      if(c == end)
        c = buffer;

      do {
        acc = o(acc, *c++);
        if(c == end)
          c = buffer;
      } while(c != start);
      return acc;
    }

    static T T_times(T &x, T &y) { return x * y; }
    T prod() const { return op(T_times); }
  };
}

#endif
