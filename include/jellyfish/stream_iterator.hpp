/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/


#ifndef __STREAM_ITERATOR_HPP__
#define __STREAM_ITERATOR_HPP__

#include <assert.h>

#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <memory>

#include <jellyfish/err.hpp>

namespace jellyfish {
/// Transform an iterator of paths (c string: const char*) into an
/// iterator of std::ifstream. Every file is opened and closed in
/// turn. The object instantiated with no argument is the end marker.
template<typename PathIterator>
class stream_iterator : public std::iterator<std::forward_iterator_tag, std::ifstream> {
  PathIterator                 begin_, end_;
  std::ifstream*               stream_;
public:
  stream_iterator(PathIterator begin, PathIterator end) :
    begin_(begin), end_(end), stream_(0)
  {
    if(begin_ != end_) {
      stream_ = new std::ifstream;
      open_file();
    }
  }
  stream_iterator(const stream_iterator& rhs) :
  begin_(rhs.begin_), end_(rhs.end_), stream_(rhs.stream_)
  { }
  stream_iterator() : begin_(), end_(), stream_() { }

  bool operator==(const stream_iterator& rhs) const {
    return stream_ == rhs.stream_;
  }
  bool operator!=(const stream_iterator& rhs) const {
    return stream_ != rhs.stream_;
  }

  std::ifstream& operator*() { return *stream_; }
  std::ifstream* operator->() { return stream_; }

  stream_iterator& operator++() {
    stream_->close();
    if(++begin_ != end_) {
      open_file();
    } else {
      delete stream_;
      stream_ = 0;
    }

    return *this;
  }
  stream_iterator operator++(int) {
    stream_iterator res(*this);
    ++*this;
    return res;
  }

protected:
  void open_file() {
    stream_->open(*begin_);
    if(stream_->fail())
      throw std::runtime_error(err::msg() << "Failed to open file '" << *begin_ << "'");
  }
};

}

#endif /* __STREAM_ITERATOR_HPP__ */
