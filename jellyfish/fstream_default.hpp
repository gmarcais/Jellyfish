/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_FSTREAM_WITH_DEFAULT_HPP__
#define __JELLYFISH_FSTREAM_WITH_DEFAULT_HPP__

#include <iostream>
#include <fstream>

template<typename Base, std::ios_base::openmode def_mode>
class fstream_default : public Base {
  typedef Base super;
  static std::streambuf* open_file(const char* str, std::ios_base::openmode mode) {
    std::filebuf* fb = new std::filebuf;
    return fb->open(str, mode);
  }

  static std::streambuf* get_streambuf(const char* str, Base& def, 
                                       std::ios_base::openmode mode) {
    return (str != 0) ? open_file(str, mode) : def.rdbuf();
  }
  static std::streambuf* get_streambuf(const char* str, std::streambuf* buf, 
                                       std::ios_base::openmode mode) {
    return (str != 0) ? open_file(str, mode) : buf;
  }

  bool do_close;
public:
  fstream_default(const char* str, Base& def, std::ios_base::openmode mode = def_mode) :
    Base(get_streambuf(str, def, mode)), do_close(str != 0) { 
    if(Base::rdbuf() == 0)
      Base::setstate(std::ios_base::badbit);
  }
  fstream_default(const char* str, std::streambuf* def, std::ios_base::openmode mode = def_mode) :
    Base(get_streambuf(str, def, mode)), do_close(str != 0) {
    if(Base::rdbuf() == 0)
      Base::setstate(std::ios_base::badbit);
  }

  ~fstream_default() {
    if(do_close) {
      delete Base::rdbuf(0);
      do_close = false;
    }
  }
  // Close is a noop at this point as GCC 4.4 has a problem with
  // Base::rdbuf in methods (breaks strict aliasing). Beats me! I
  // think it is a false positive.
  void close() {} 
};

typedef fstream_default<std::ostream, std::ios_base::ios_base::out> ofstream_default;
typedef fstream_default<std::istream, std::ios_base::ios_base::in>  ifstream_default;

#endif // __JELLYFISH_FSTREAM_WITH_DEFAULT_HPP__
