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

#ifndef __ERR_HPP__
#define __ERR_HPP__

#include <iostream>
#include <iomanip>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

namespace err {
  class code {
    int _code;
  public:
    code(int c) : _code(c) {}
    int get_code() const { return _code; }
  };

  class no_t {
  public:
    no_t() {}
    static void write(std::ostream &os, int e) {
      char err_str[4096];
      strerror_r(e, err_str, sizeof(err_str));
      os << ": " << err_str;
    }
  };
  static const no_t no;
  std::ostream &operator<<(std::ostream &os, const err::no_t &x);

  class substr {
    const char  *_s;
    const size_t _l;
  public:
    substr(const char *s, size_t len) : _s(s), _l(len) {}
    friend std::ostream &operator<<(std::ostream &os, const substr &ss);
  };

  class die_t {
    int _code;
    int _errno;
  public:
    die_t() : _code(1), _errno(errno) {}
    die_t(int c) : _code(c), _errno(errno) {}
    ~die_t() {
      std::cerr << std::endl;
      exit(_code);
    }

    die_t & operator<<(const code &x) {
      _code = x.get_code();
      return *this;
    }
    die_t & operator<<(const no_t &x) {
      x.write(std::cerr, _errno);
      return *this;
    }
    die_t & operator<<(const char *a[]) {
      for(int i = 0; a[i]; i++)
        std::cerr << (i ? "\n" : "") << a[i];
      return *this;
    }
    die_t & operator<<(const std::exception &e) {
      std::cerr << e.what();
      return *this;
    }
    template<typename T>
    die_t & operator<<(const T &x) {
      std::cerr << x;
      return *this;
    }
  };

  template<typename err_t>
  class raise_t {
    int                _errno;
    std::ostringstream oss;
  public:
    raise_t() : _errno(errno) {}
    ~raise_t() { throw err_t(oss.str()); }

    raise_t & operator<<(const no_t &x) {
      x.write(oss, _errno);
      return *this;
    }
    template<typename T>
    raise_t & operator<<(const T &x) {
      oss << x;
      return *this;
    }
  };
}


#define die if(1) err::die_t()
#define eraise(e) if(1) err::raise_t<e>()
#define define_error_class(name)                                    \
  class name : public std::runtime_error {                          \
  public: name(const std::string &txt) : std::runtime_error(txt) {} \
  }

#endif
