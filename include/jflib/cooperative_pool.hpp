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


#ifndef __JELLYFISH_JFLIB_COOPERATIVE_POOL_HPP__
#define __JELLYFISH_JFLIB_COOPERATIVE_POOL_HPP__

#include <jflib/circular_buffer.hpp>
#include <jflib/compare_and_swap.hpp>
/// Cooperative pool

namespace jellyfish { namespace jflib {
template<typename D, typename T>
class cooperative_pool {
  typedef jflib::circular_buffer<uint32_t> cbT;
  typedef T                                element_type;

  uint32_t      size_;
  element_type* elts_;
  cbT           cons_prod_;     // FIFO from Consumers to Producers
  cbT           prod_cons_;     // FIFO from Producers to Consumers
  int           has_producer_;  // Tell whether a thread is acting as a producer

  // Small class with RAII. that take a token in an atomic fashion (like
  // has_producer_). Release token when going out of scope.
  class take_token {
    int* const token_;
    const bool has_token_;
  public:
    take_token(int* token) : token_(token), has_token_(cas(token_, 0, 1)) { }
    ~take_token() {
      if(has_token_)
        jflib::a_store(token_, 0);
    }
    bool has_token() const { return has_token_; }
  };

public:
  cooperative_pool(uint32_t size) :
    size_(size),
    elts_(new element_type[size_]),
    cons_prod_(size),
    prod_cons_(size),
    has_producer_(0)
  {
    // Every element is empty and ready to be filled by the producer
    for(size_t i = 0; i < size_; ++i)
      cons_prod_.enqueue(i);
  }

  uint32_t size() const { return size_; }

  // Contains a filled element or is empty. In which case the producer
  // is done and we should stop processing.
  class job {
    cooperative_pool& cp_;
    uint32_t          i_;       // Index of element
  public:
    job(cooperative_pool& cp) : cp_(cp), i_(cp_.get_element()) { }
    ~job() { release(); }

    void release() {
      if(!is_empty())
        cp_.cons_prod_.enqueue(i_);
    }
    bool is_empty() const { return i_ == cbT::guard; }

    element_type& operator*() { return cp_.elts_[i_]; }
    element_type* operator->() { return &cp_.elts_[i_]; }

  private:
    // Disable copy of job
    job(const job& rhs) { }
    job& operator=(const job& rhs) { }
  };
  friend job;

private:
  enum PRODUCER_STATUS { PRODUCER_PRODUCED, PRODUCER_DONE, PRODUCER_EXISTS };
  uint32_t get_element() {
    int iteration = 0;

    while(true) {
      uint32_t i = prod_cons_.dequeue();
      if(i != cbT::guard)
        return i;

      // Try to become producer
      switch(become_producer()) {
      case PRODUCER_PRODUCED:
        iteration = 0;
        break;
      case PRODUCER_DONE:
        return cbT::guard;
      case PRODUCER_EXISTS:
        delay(iteration);
      }
    }
  }

  PRODUCER_STATUS become_producer() {
    // Mark that we have a produce (myself). If not, return. Token
    // will be release automatically at end of method
    take_token producer_token(&has_producer_);
    if(!producer_token.has_token())
      return PRODUCER_EXISTS;

    if(prod_cons_.is_closed())
      return PRODUCER_DONE;

    uint32_t i;
    try {
      while(true) {
        i = cons_prod_.dequeue();
        if(i == cbT::guard)
          return PRODUCER_PRODUCED;

        bool done = static_cast<D*>(this)->produce(elts_[i]);
        if(done) {
          cons_prod_.enqueue(i);
          prod_cons_.close();
          return PRODUCER_DONE;
        }

        prod_cons_.enqueue(i);
      }
    } catch(...) {
      // Is threw an exception -> same as being done
      cons_prod_.enqueue(i);
      prod_cons_.close();
      return PRODUCER_DONE;
    }

    return PRODUCER_PRODUCED; // never reached
  }

  // First 16 operations -> no delay. Then exponential back-off up to a second.
  void delay(int iteration) {
    if(iteration < 16)
      return;
    int shift = 10 - std::min(iteration - 16, 10);
    usleep((1000000 - 1) >> shift);
  }
};

} } // namespace jellyfish { namespace jflib {
#endif /* __JELLYFISH_JFLIB_COOPERATIVE_POOL_HPP__ */
