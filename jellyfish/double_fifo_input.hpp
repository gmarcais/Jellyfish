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

#ifndef __JELLYFISH_DOUBLE_FIFO_INPUT__
#define __JELLYFISH_DOUBLE_FIFO_INPUT__

#include <jellyfish/concurrent_queues.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/dbg.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/time.hpp>

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <assert.h>

namespace jellyfish {
  /* Double lock free fifo containing elements of type T. 
   *
   * The input thread, created by this class, runs the virtual
   * function 'fill' to fill up tockens from the wq (to be Written
   * tocken Queue) and append then into the rq (to be Read tocken
   * Queue).
   *
   * next() returns a pointer to a filled tocken. If the queue is
   * empty, it will sleep some (start with 1/100th +/- 50%, and
   * exponential back off) to give time for the queue to fill up.
   */
  template<typename T>
  class double_fifo_input {
    define_error_class(Error);
    typedef concurrent_queue<T> queue;

    // The only transitions are:
    // WORKING -> SLEEPING -> WAKENING -> WORKING
    enum state_t { WORKING, SLEEPING, WAKENING };

    queue                rq, wq;
    T *                  buckets;
    const unsigned long  nb_buckets;
    state_t volatile     state;
    pthread_t            input_id;
    locks::pthread::cond full_queue;

    static void *static_input_routine(void *arg);
    void         input_routine();
    
  public:
    typedef T bucket_t;
    explicit double_fifo_input(unsigned long _nb_buckets);
    virtual ~double_fifo_input();

    virtual void fill() = 0;
    T *next();
    void release(T *bucket);
    bool is_closed() { return rq.is_closed(); }

    typedef T *bucket_iterator;
    bucket_iterator bucket_begin() const { return buckets; }
    bucket_iterator bucket_end() const { return buckets + nb_buckets; }

  protected:
    // Get bucket to fill and release.
    T *write_next();
    void write_release(T *bucket);
    void close() { rq.close(); }

  private:
    // Wake up input thread if it was sleeping. Returns previous
    // state.
    state_t input_wake();
  };

  /****/

  template<typename T>
  double_fifo_input<T>::double_fifo_input(unsigned long _nb_buckets) :
    rq(_nb_buckets), wq(_nb_buckets), nb_buckets(_nb_buckets), state(WORKING),
    input_id(0)
  {
    buckets = new T[nb_buckets];

    for(unsigned long i = 0; i < nb_buckets; ++i)
      wq.enqueue(&buckets[i]);

    if(pthread_create(&input_id, 0, static_input_routine, (void *)this) != 0)
      eraise(Error) << "Failed creating input thread" << err::no;
  }

  template<typename T>
  double_fifo_input<T>::~double_fifo_input() {
    if(input_id)
      if(pthread_cancel(input_id)) {
        void *input_return;
        pthread_join(input_id, &input_return);
      }
    delete [] buckets;
  }

  template<typename T>
  void *double_fifo_input<T>::static_input_routine(void *arg) {
    double_fifo_input *o = (double_fifo_input *)arg;
    o->input_routine();
    return 0;
  }

  template<typename T>
  void double_fifo_input<T>::input_routine() {
    state_t prev_state;

    while(!rq.is_closed()) {
      // The write queue is full or this is the first iteration, sleep
      // until it become less than some threshold
      full_queue.lock();
      prev_state = atomic::gcc::cas(&state, WORKING, SLEEPING);
      assert(prev_state == WORKING);
      do {
        full_queue.wait();
      } while(state != WAKENING);
      prev_state = atomic::gcc::cas(&state, WAKENING, WORKING);
      assert(prev_state == WAKENING);
      full_queue.unlock();

      fill();
    }
  }

  template<typename T>
  typename double_fifo_input<T>::state_t double_fifo_input<T>::input_wake() {
    state_t prev_state = atomic::gcc::cas(&state, SLEEPING, WAKENING);
    assert(prev_state >= WORKING && prev_state <= WAKENING);
    if(prev_state == SLEEPING) {
      full_queue.lock();
      full_queue.signal();
      full_queue.unlock();
    }
    return prev_state;
  }

  template<typename T>
  T *double_fifo_input<T>::next() {
    if(rq.is_low()) // && !rq.is_closed())
      input_wake();
  
    T *res = 0;
    while(!(res = rq.dequeue())) {
      if(rq.is_closed())
        return 0;
      input_wake();
      // TODO Should we wait on a lock instead when the input thread is
      // already in working state (i.e. it is most likely blocked on
      // some I/O).
      static struct timespec time_sleep = { 0, 10000000 };
      nanosleep(&time_sleep, NULL);
    }

    return res;
  }

  template<typename T>
  void double_fifo_input<T>::release(T *bucket) {
    assert(bucket - buckets >= 0 && (unsigned long)(bucket - buckets) < nb_buckets);
    wq.enqueue(bucket);
  }

  template<typename T>
  T *double_fifo_input<T>::write_next() {
    return wq.dequeue();
  }

  template<typename T>
  void double_fifo_input<T>::write_release(T *bucket) {
    assert(bucket - buckets >= 0 && (unsigned long)(bucket - buckets) < nb_buckets);
    rq.enqueue(bucket);
  }
}

#endif
