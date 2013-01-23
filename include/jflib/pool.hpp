/*  This file is part of Jflib.

    Jflib is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jflib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jflib.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef _JFLIB_POOL_H_
#define _JFLIB_POOL_H_

#include <jellyfish/circular_buffer.hpp>
#include <jflib/locks_pthread.hpp>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace jflib {
  /** Fixed size pool of objects with lock-free access. Conceptually,
   * there are two sides: A and B. There are 2 FIFOs in opposite
   * direction between A and B. One "get" an element from one side
   * (say A) and then "release" it. This correspond to dequeue from
   * the queue B->A then enqueueing into A->B. To get an element can
   * block if the queue is empty but to release never does.
   *
   * The class itself manages 'size' elements in the queues and are
   * initialized with 'value'. Initially, all the elements are in the
   * B->A queue.
   *
   * The elements of type T in the pool should not be accessed
   * unless one has the obtained the elt object and has not released
   * (or destructed) it.
   *
   * WARNING: The iterator on the elements does not check any of this
   * and is not thread-safe. It is designed to further
   * inspect/initialize the elements in serial mode.
   */
template<typename T, typename CV = locks::pthread::cond>
  class pool {
  public:
    class                           side;

    typedef T* iterator;

    pool(size_t size) :
      size_(size), elts_(new T[size]), B2A(size, elts_), A2B(size, elts_)
    {
      B2A.other_ = &A2B;
      A2B.other_ = &B2A;
      for(uint32_t i = 0; i < size_; ++i)
        B2A.fifo_.enqueue(i);
    }
    virtual ~pool() {
      delete [] elts_;
    }

    /** Size of pool */
    size_t size() const { return size_; }

    /** Get the side A. Used to get an element from side A. */
    side &get_A() { return B2A; }
    /** Get the side B. Used to get an element from side B. */
    side &get_B() { return A2B; }
    /** Close FIFO from A to B */
    void close_A_to_B() { A2B.fifo_.close(); A2B.signal(true); }
    /** Close FIFO from B to A */
    void close_B_to_A() { B2A.fifo_.close(); B2A.signal(true); }
    /** Check whether FIFO from A to B is closed */
    bool is_closed_A_to_B() const { return A2B.fifo_.is_closed(); }
    /** Check whether FIFO from B to A is closed */
    bool is_closed_B_to_A() const { return B2A.fifo_.is_closed(); }


    /** Iterators on the elements. The iterator visits every element
        manage in the pool in an order independent from two FIFOs. The
        operation is NOT thread safe.
     */
    T* begin() { return elts_; }
    T* end() { return elts_ + size_; }

    /** A wrapper around an element of type T. The element can be
     * obtained with `operator*` or `operator->`. `release()` is called by
     * the destructor or can be called manually to requeue the element
     * in to the double fifo. When the double fifo is empty and
     * closed, the element obtained is empty (i.e. the `is_empty()`
     * method returns true).
     *
     * The element can only be initialized/constructed with a side
     * (obtained by `get_A()` or `get_B()` from the pool class), which
     * effectively dequeues an element of type `T` from that FIFO.
     */
    class elt {
    public:
      elt() : i_(cbT::guard), v_(0), s_(0) { }
      /** Constructor from a side. Dequeue from that side. */
      elt(side &s) : i_(s.get()), v_(s[i_]), s_(s.other_) { }
      ~elt() { release(); }
      /** Initialize from a side. Release if not empty and dequeue
          from the given side */
      elt &operator=(side &s) {
        release();
        i_ = s.get();
        v_ = s[i_];
        s_ = s.other_;
        return *this;
      }

      void release() {
        if(v_)
          s_->release(i_);
        v_ = 0;
      }
      bool is_empty() const { return v_ == 0; }
      /** Dereference element */
      T &operator*() { return *v_; }
      /** Dereference element */
      T *operator->() { return v_; }

      friend class pool;
    private:
      elt(const elt &rhs) { }
      elt &operator=(const elt &rhs) { }

      uint32_t  i_;             // Index of stored value
      T        *v_;             // Stored value
      side     *s_;             // Side to release to
    };
    static const elt closed;

  protected:
  typedef circular_buffer<uint32_t> cbT;
  public:
    /** A circular buffer with a conditional variable to wait in the
     * empty event and a pointer to the other direction circular
     * buffer. Every method is private and are accessible only by its
     * friends :)
     */
    class side {
    private:
      friend class pool;
      friend class elt;
      enum State { NONE, WAITING, CLOSED };
      side(size_t size, T* elts) :
        fifo_(2*size), state_(NONE), other_(0), elts_(elts) { }

      uint32_t get();
      T *operator[](uint32_t i);
      void release(uint32_t i);
      void signal(bool force = false);

      cbT   fifo_;
      CV    cond_;
      State state_;
      side *other_;
      T    *elts_;
    };

    size_t size_;
    T*     elts_;
    side B2A;                     // Manages queue from B->A
    side A2B;                     // Manages queue from A->B
  };
}

template<typename T, typename CV>
uint32_t jflib::pool<T, CV>::side::get() {
  bool     last_attempt = false;
  uint32_t res          = fifo_.dequeue();
  while(res == cbT::guard) {
    cond_.lock();

    switch(a_load(state_)) {
    case CLOSED:
      if(last_attempt) {
        cond_.unlock();
        return cbT::guard;
      } else {
        last_attempt = true;
        break;
      }
    case NONE:
      a_store(state_, WAITING);
      break;
    case WAITING:
      break;
    }
    res = fifo_.dequeue();
    if(res == cbT::guard) {
      if(last_attempt) {
        cond_.unlock();
        break;
      }
    } else {
      cond_.unlock();
      break;
    }
    do {
      cond_.timedwait(5);
    } while(a_load(state_) == WAITING);
    cond_.unlock();
  }

  return res;
}

template<typename T, typename CV>
T * jflib::pool<T, CV>::side::operator[](uint32_t i) {
  if(i == cbT::guard)
    return 0;
  return &elts_[i];
}

template<typename T, typename CV>
void jflib::pool<T, CV>::side::release(uint32_t i) {
  while(!fifo_.enqueue(i)) ;
  signal();
}

template<typename T, typename CV>
void jflib::pool<T, CV>::side::signal(bool close) {
  if(a_load(state_) != NONE || close) {
    cond_.lock();
    a_store(state_, close ? CLOSED : NONE);
    cond_.broadcast();
    cond_.unlock();
  }
}

#endif /* _JFLIB_POOL_H_ */
