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

#ifndef __DBG_HPP__
#define __DBG_HPP__

#include <iostream>
#include <iomanip>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

namespace dbg {
#ifdef DEBUG

  class substr {
    const char  *_s;
    const size_t _l;
  public:
    substr(const char *s, size_t len) : _s(s), _l(len) {}
    friend std::ostream &operator<<(std::ostream &os, const substr &ss);
  };

  class print_t {
    static pthread_mutex_t _lock;

    std::ostringstream     _buff;
  public:
    print_t(const char *file, const char *function, int line) {
      _buff << pthread_self() << ":" << basename(file) << ":" 
            << function << ":" << line << ": ";
    }

    ~print_t() {
      pthread_mutex_lock(&_lock);
      std::cerr << _buff.str() << std::endl;
      pthread_mutex_unlock(&_lock);
    }

    print_t & operator<<(const char *a[]) {
      for(int i = 0; a[i]; i++)
        _buff << (i ? "\n" : "") << a[i];
      return *this;
    }
    print_t & operator<<(const std::exception &e) {
      _buff << e.what();
      return *this;
    }
    template<typename T>
    print_t & operator<<(const T &x) {
      _buff << x;
      return *this;
    }
  };

#else // !DEBUG

  class substr {
  public:
    substr(const char *s, size_t len) { }
    friend std::ostream &operator<<(std::ostream &os, const substr &ss);
  };

  class print_t {
  public:
    print_t(const char *file, const char *function, int line) {}
    
    print_t & operator<<(const char *a[]) { return *this; }
    print_t & operator<<(const std::exception &e) { return *this; }
    template<typename T>
    print_t & operator<<(const T &x) { return *this; }
  };

#endif // DEBUG
}



#define DBG if(1) dbg::print_t(__FILE__, __FUNCTION__, __LINE__)
#define V(v) " " #v ":" << v

#endif
