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

#ifndef __JELLYFISH_ERR_HPP__
#define __JELLYFISH_ERR_HPP__

#include <iostream>
#include <iomanip>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cerrno>

namespace jellyfish {
namespace err {
struct msg {
  std::ostringstream msg_;

  msg() { }
  explicit msg(const std::exception& e) { *this << e; }
  template<typename T>
  explicit msg(const T& x) { *this << x; }

  operator std::string() const { return msg_.str(); }

  template<typename T>
  msg& operator<<(const T& x) {
    msg_ << x;
    return *this;
  }

  msg& operator<<(const std::exception& e) {
    msg_ << e.what();
    // try {
    //   std::rethrow_if_nested(e);
    // } catch (const std::exception& nested) {
    //   msg_ << '\n';
    //   return *this << nested;
    // }
    return *this;
  }

  msg& operator<<(msg& (*pf)(msg&)) { return pf(*this); }

};

// Select the correct version (GNU or XSI) version of
// ::strerror_r. err::strerror_ behaves like the GNU version of strerror_r,
// regardless of which version is provided by the system.
inline const char* strerror__(char* buf, int res) {
  return res != -1 ? buf : "error";
}
inline const char* strerror__(char* buf, char* res) {
  return res;
}
inline const char* strerror_r(int err, char* buf, size_t buflen) {
  return strerror__(buf, ::strerror_r(err, buf, buflen));
}

inline std::ostream& no(std::ostream& os) {
  char buf[128];
  return os << strerror_r(errno, buf, sizeof(buf));
}

inline msg& no(msg& m) {
  char buf[128];
  return m << strerror_r(errno, buf, sizeof(buf));
}

inline void die(int code, std::string msg) {
  std::cerr << msg << '\n';
  exit(code);
}

inline void die(std::string msg) { die(1, msg); }
} // namespace err
} // namespace jellyfish

#define define_error_class(name)                                        \
  class name : public std::runtime_error {                              \
  public: explicit name(const std::string &txt) : std::runtime_error(txt) {} \
  }

#endif // __JELLYFISH_ERR_HPP__
