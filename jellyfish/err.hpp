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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_DECL_STRERROR_R == 0
#ifdef STRERROR_R_CHAR_P
extern char* strerror_r(int, char*, size_t);
#else
extern int strerror_r(int, char*, size_t);
#endif
#endif

namespace err {
  class code {
    int _code;
  public:
    explicit code(int c) : _code(c) {}
    int get_code() const { return _code; }
  };

  class no_t {
  public:
    no_t() {}
    static void write(std::ostream &os, int e) {
    char  err_str[1024];
    char* str;

#ifdef STRERROR_R_CHAR_P
    str = strerror_r(e, err_str, sizeof(err_str));
    if(!str) { // Should never happen
      strcpy(err_str, "error");
    str = err_str;
    }
#else
    int err = strerror_r(e, err_str, sizeof(err_str));
    if(err)
      strcpy(err_str, "error");
      str = err_str;
#endif
    os << ": " << str;
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
    explicit die_t(int c) : _code(c), _errno(errno) {}
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
#define define_error_class(name)                                        \
  class name : public std::runtime_error {                              \
  public: explicit name(const std::string &txt) : std::runtime_error(txt) {} \
  }

#endif
