#ifndef __STREAM_ITERATOR_HPP__
#define __STREAM_ITERATOR_HPP__

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
///
/// The internal stream is kept in an auto_ptr. After the
/// stream_iterator has been copied, the original variable is at the
/// end position (i.e. equal to stream_iterator()).
template<typename PathIterator>
class stream_iterator : public std::iterator<std::forward_iterator_tag, std::ifstream> {
  PathIterator  begin_, end_;
  std::ifstream stream_;
public:
  stream_iterator(PathIterator begin, PathIterator end) :
    begin_(begin), end_(end)
  { ++*this; }
  stream_iterator(const stream_iterator& rhs) :
  begin_(rhs.begin_), end_(rhs.end_)
  { }
  stream_iterator() : begin_(), end_() { }

  bool operator==(const stream_iterator& rhs) const { return begin_ == rhs.begin_; }
  bool operator!=(const stream_iterator& rhs) const { return begin_ != rhs.begin_; }

  std::ifstream& operator*() { return stream_; }
  std::ifstream* operator->() { return &stream_; }

  stream_iterator& operator++() {
    stream_.close();
    if(begin_ != end_) {
      stream_.open(*begin_);
      if(stream_.fail())
        eraise(std::runtime_error) << "Failed to open file '" << *begin_ << "'";
      ++begin_;
    } else {
      begin_ = end_ = PathIterator();
    }
    return *this;
  }
  stream_iterator operator++(int) {
    stream_iterator res(*this);
    ++*this;
    return res;
  }
};

}

#endif /* __STREAM_ITERATOR_HPP__ */
