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

#ifndef __JELLYFISH_TIME_HPP__
#define __JELLYFISH_TIME_HPP__

#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <iomanip>

class Time {
  static const suseconds_t max_useconds = 1000000UL;
  struct timeval tv;

 public:
  static const Time zero;
  Time(bool init = true) {
    if(init)
      now();
  }
  Time(time_t sec, suseconds_t usec) {
    tv.tv_sec  = sec;
    tv.tv_usec = usec;
  }
  Time &operator=(const Time &o) {
    if(&o != this) {
      tv.tv_sec = o.tv.tv_sec;
      tv.tv_usec = o.tv.tv_usec;
    }
    return *this;
  }
  Time &operator+=(const Time &o) {
    return *this = *this + o;
  }

  Time operator-(const Time &o) const {
    time_t sec = tv.tv_sec - o.tv.tv_sec;
    suseconds_t usec;
    if(o.tv.tv_usec > tv.tv_usec) {
      usec = (max_useconds + tv.tv_usec) - o.tv.tv_usec;
      sec--;
    } else {
      usec = tv.tv_usec - o.tv.tv_usec;
    }
    return Time(sec, usec);
  }

  Time operator+(const Time &o) const {
    time_t sec = tv.tv_sec + o.tv.tv_sec;
    suseconds_t usec = tv.tv_usec + o.tv.tv_usec;
    if(usec >= max_useconds) {
      sec++;
      usec -= max_useconds;
    }
    return Time(sec, usec);
  }

  void now() { gettimeofday(&tv, NULL); }

  std::string str() const {
    std::ostringstream res;
    res << tv.tv_sec << "."
        << std::setfill('0') << std::setw(6) << std::right << tv.tv_usec;
    return res.str();
  }
};

#endif // __TIME_HPP__
