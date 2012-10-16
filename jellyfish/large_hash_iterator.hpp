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

#ifndef __LARGE_HASH_ITERATOR_HPP__
#define __LARGE_HASH_ITERATOR_HPP__

#include <iterator>
#include <utility>


namespace jellyfish { namespace large_hash {
/// STL like iterator on a large hash array.
template<typename array>
class stl_iterator :
    public std::iterator<std::forward_iterator_tag, typename array::value_type>,
    public array::lazy_iterator
{
public:
  typedef typename array::key_type    key_type;
  typedef typename array::mapped_type mapped_type;
  typedef typename array::value_type  value_type;

protected:
  typedef typename array::lazy_iterator           lit;
  typedef std::pair<key_type&, mapped_type> pair;
  pair val_;

public:
  explicit stl_iterator(array* ary, size_t id = 0) : lit(ary, id, ary->size()), val_(lit::key_, (mapped_type)0)
  { ++*this; }
  explicit stl_iterator() : lit(0, 0, 0), val_(lit::key_, (mapped_type)0) { }
  stl_iterator(const stl_iterator& rhs) : lit(rhs), val_(lit::key_, rhs.val_.second) { }

  bool operator==(const stl_iterator& rhs) const { return lit::ary_ == rhs.ary_ && lit::id_ == rhs.id_; }
  bool operator!=(const stl_iterator& rhs) const { return !(*this == rhs); }

  const value_type& operator*() {
    lit::key();
    val_.second = lit::val();
    return val_;
  }
  const value_type* operator->() { return &this->operator*(); }

  stl_iterator& operator++() {
    if(!lit::next()) {
      lit::ary_ = 0;
      lit::id_  = 0;
    }
    return *this;
  }

  stl_iterator operator++(int) {
    stl_iterator res(*this);
    ++*this;
    return res;
  }
};

} } // namespace jellyfish { namespace large_hash {
#endif /* __LARGE_HASH_ITERATOR_HPP__ */
