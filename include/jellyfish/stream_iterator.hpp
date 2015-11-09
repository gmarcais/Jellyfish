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
class stream_iterator : public std::iterator<std::input_iterator_tag, std::ifstream> {
  PathIterator begin_, end_;
  std::filebuf stream_;

public:
  stream_iterator(PathIterator begin, PathIterator end) :
    begin_(begin), end_(end)
  {
      open_file();
  }
  stream_iterator(PathIterator begin)
    : begin_(begin), end_(begin)
  { }
  // stream_iterator(const stream_iterator& rhs)
  //   : begin_(rhs.begin_), end_(rhs.end_), stream_(rhs.stream_)
  // { }
  stream_iterator(stream_iterator&& rhs) = default;
  stream_iterator() : begin_(PathIterator()), end_(begin_) { }

  bool operator==(const stream_iterator& rhs) const {
    return begin_ == rhs.begin_;
  }
  bool operator!=(const stream_iterator& rhs) const {
    return begin_ != rhs.begin_;
  }

  std::filebuf* operator*() { return &stream_; }
  std::filebuf** operator->() { return &this->operator*(); }

  stream_iterator& operator++() {
    stream_.close();
    ++begin_;
    open_file();
    return *this;
  }
  stream_iterator operator++(int) {
    stream_iterator res(*this);
    ++*this;
    return res;
  }

protected:
  void open_file() {
    if(begin_ == end_) return;
    stream_.open(*begin_, std::ios::in);
    if(!stream_.is_open())
      throw std::runtime_error(err::msg() << "Failed to open file '" << *begin_ << "'");
  }
};

}

#endif /* __STREAM_ITERATOR_HPP__ */
