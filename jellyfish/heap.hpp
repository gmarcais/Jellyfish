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

#ifndef __JELLYFISH_HEAP_HPP__
#define __JELLYFISH_HEAP_HPP__

#include <algorithm>

namespace jellyfish {
  template<typename iterator>
  struct heap_item_t {
    uint64_t       key;
    uint64_t       val;
    uint64_t       pos;
    iterator      *it;

    heap_item_t() : key(0), val(0), pos(0) { }

    explicit heap_item_t(iterator &iter) { 
      initialize(iter);
    }

    void initialize(iterator &iter) {
      key = iter.key;
      val = iter.val;
      pos = iter.get_pos();
      it  = &iter;
    }

    // STL make_heap creates a max heap. We want a min heap, so
    // reverse comparator!
    bool operator<(const heap_item_t & other) {
      if(pos == other.pos)
        return key > other.key;
      return pos > other.pos;
    }
  };

  template<typename iterator>
  class heap_item_compare {
  public:
    inline bool operator() (heap_item_t<iterator> *i1, heap_item_t<iterator> *i2) {
      return *i1 < *i2;
    }
  };

  template<typename iterator>
  class heap_t {
    heap_item_t<iterator>       *storage;
    heap_item_t<iterator>      **elts;
    size_t                       capacity_;
    size_t                       h;
    heap_item_compare<iterator>  compare;
  public:
    typedef const heap_item_t<iterator> *const_item_t;

    heap_t() : storage(0), elts(0), capacity_(0), h(0) { }
    explicit heap_t(size_t _capacity)  { initialize(_capacity); }
    ~heap_t() {
      delete[] storage;
      delete[] elts;
    }

    void initialize(size_t _capacity) {
      capacity_ = _capacity;
      h = 0;
      storage = new heap_item_t<iterator>[capacity_];
      elts = new heap_item_t<iterator>*[capacity_];
      for(size_t h1 = 0; h1 < capacity_; ++h1)
        elts[h1] = &storage[h1];
    }

    void fill(iterator &it) {
      h = 0;
      while(h < capacity_) {
        if(!it.next())
          break;
        storage[h].initialize(it);
        elts[h] = &storage[h];
        h++;
      }
      std::make_heap(elts, elts + h, compare);
    }
    template<typename ForwardIterator>
    void fill(ForwardIterator first, ForwardIterator last) {
      h = 0;
      while(h < capacity_ && first != last) {
        if(!first->next())
          break;
        storage[h].initialize(*first++);
        elts[h] = &storage[h];
        h++;
      }
      std::make_heap(elts, elts + h, compare);
    }
    
    bool is_empty() const { return h == 0; }
    bool is_not_empty() const { return h > 0; }
    size_t size() const { return h; }
    size_t capacity() const { return capacity_; }

    // The following 3 should only be used after fill has been called
    const_item_t head() const { return elts[0]; }
    void pop() { std::pop_heap(elts, elts + h--, compare); }
    void push(iterator &item) {
      elts[h]->initialize(item);
      std::push_heap(elts, elts + ++h, compare);
    }
  };
}

#endif // __HEAP_HPP__
