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

#include <time.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <assert.h>

class Time {
  static const long max_nseconds = 1000000000UL;
  const clockid_t _type;
  struct timespec _tp;

 public:
  static const Time zero;
  Time(bool init = true, clockid_t type = CLOCK_MONOTONIC) : _type(type) {
    if(init)
      now();
  }
  Time(time_t sec, long nsec, clockid_t type = CLOCK_MONOTONIC) : _type(type) {
    _tp.tv_sec  = sec;
    _tp.tv_nsec = nsec;
  }
  Time &operator=(const Time &o) {
    if(&o != this) {
      _tp.tv_sec  = o._tp.tv_sec;
      _tp.tv_nsec = o._tp.tv_nsec;
    }
    return *this;
  }

  Time operator-(const Time &o) const {
    time_t sec = _tp.tv_sec - o._tp.tv_sec;
    long   nsec;
    if(o._tp.tv_nsec > _tp.tv_nsec) {
      nsec = (max_nseconds + _tp.tv_nsec) - o._tp.tv_nsec;
      sec--;
    } else {
      nsec = _tp.tv_nsec - o._tp.tv_nsec;
    }
    return Time(sec, nsec);
  }

  Time & operator+=(const Time &o) {
    _tp.tv_sec  += o._tp.tv_sec;
    _tp.tv_nsec += o._tp.tv_nsec;
    if(_tp.tv_nsec >= max_nseconds) {
      ++_tp.tv_sec;
      _tp.tv_nsec -= max_nseconds;
    }
    assert(_tp.tv_nsec >= 0);
    assert(_tp.tv_nsec < max_nseconds);
    return *this;
  }

  const Time operator+(const Time &o) const {
    return Time(*this) += o;
  }

  void now() { clock_gettime(_type, &_tp); }
  Time elapsed() const {
    Time t(true, _type);
    return t - *this;
  }

  std::string str() const {
    std::ostringstream res;
    res << _tp.tv_sec << "."
        << std::setfill('0') << std::setw(9) << std::right << _tp.tv_nsec;
    return res.str();
  }
};

class Monotonic : public Time {
public:
  Monotonic(bool init = true) : Time(init, CLOCK_MONOTONIC) {}
  Monotonic(time_t sec, long nsec) : Time(sec, nsec, CLOCK_MONOTONIC) {}
};
class ProcCPU : public Time {
public:
  ProcCPU(bool init = true) : Time(init, CLOCK_PROCESS_CPUTIME_ID) {}
  ProcCPU(time_t sec, long nsec) : Time(sec, nsec, CLOCK_PROCESS_CPUTIME_ID) {}
};
class ThreadCPU : public Time {
public:
  ThreadCPU(bool init = true) : Time(init, CLOCK_THREAD_CPUTIME_ID) {}
  ThreadCPU(time_t sec, long nsec) : Time(sec, nsec, CLOCK_THREAD_CPUTIME_ID) {}
};


#endif // __TIME_HPP__
