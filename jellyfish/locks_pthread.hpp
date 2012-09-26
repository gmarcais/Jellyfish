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

#ifndef __JELLYFISH_LOCKS_PTHREAD_HPP__
#define __JELLYFISH_LOCKS_PTHREAD_HPP__

#include <pthread.h>

namespace locks {
  namespace pthread {
    class cond
    {
      pthread_mutex_t   _mutex;
      pthread_cond_t	_cond;
      
    public:
      cond() { 
        pthread_mutex_init(&_mutex, NULL);
        pthread_cond_init(&_cond, NULL);
      }
      
      ~cond() {
        pthread_cond_destroy(&_cond);
        pthread_mutex_destroy(&_mutex);
      }
      
      inline void lock() { pthread_mutex_lock(&_mutex); }
      inline void unlock() { pthread_mutex_unlock(&_mutex); }
      inline void wait() { pthread_cond_wait(&_cond, &_mutex); }
      inline void signal() { pthread_cond_signal(&_cond); }
      inline void broadcast() { pthread_cond_broadcast(&_cond); }
    };
    
    class mutex
    {
      pthread_mutex_t     _mutex;
    
    public:
      mutex() {
        pthread_mutex_init(&_mutex, NULL);
      }
    
      ~mutex() {
        pthread_mutex_destroy(&_mutex);
      }
    
      inline void lock() { pthread_mutex_lock(&_mutex); }
      inline void unlock() { pthread_mutex_unlock(&_mutex); }
      inline bool try_lock() { return !pthread_mutex_trylock(&_mutex); }
    };
    
    class Semaphore
    {
        int _value, _wakeups;
        cond _cv;
    public:
        explicit Semaphore(int value) :
          _value(value),
          _wakeups(0)
        {
            // nothing to do
        }

        ~Semaphore() {}

        inline void wait() {
           _cv.lock();
           _value--;
           if (_value < 0) {
             do {
               _cv.wait();
             } while(_wakeups < 1);
             _wakeups--;
           }
           _cv.unlock();
        }

        inline void signal() {
           _cv.lock();
           _value++;
           if(_value <= 0) {
             _wakeups++;
             _cv.signal();
           }
           _cv.unlock();
        }
    };

#if defined(_POSIX_BARRIERS) && (_POSIX_BARRIERS - 20012L) >= 0
    class barrier
    {
      pthread_barrier_t _barrier;
      
    public:
      explicit barrier(unsigned count) {

	pthread_barrier_init(&_barrier, NULL, count);
      }
      
      ~barrier() {
	pthread_barrier_destroy(&_barrier);
      }
      
      inline int wait() {
	return pthread_barrier_wait(&_barrier);
      }
    };

#else
#  ifndef PTHREAD_BARRIER_SERIAL_THREAD
#    define  PTHREAD_BARRIER_SERIAL_THREAD 1
#  endif

    class barrier
    {
      int count; // required # of threads
      int current;    // current # of threads that have passed thru
      mutex barlock;  // protect current
      Semaphore barrier1; // implement the barrier
      Semaphore barrier2;

    public:
      explicit barrier(unsigned cnt) 
        : count(cnt), current(0), barrier1(0), barrier2(0) {
      }

      ~barrier() {}

      inline int wait() {
        int ret = 0;
        barlock.lock();
        current += 1;
        if(current == count) {
          ret = PTHREAD_BARRIER_SERIAL_THREAD;
          for(int i=0; i<count;i++) {
            barrier1.signal();
          }
        }
        barlock.unlock();
        barrier1.wait(); // wait for n threads to arrive

        barlock.lock();
        current -= 1;
        if(current == 0) {
          for(int i=0;i<count;i++) {
            barrier2.signal();
          }
        }
        barlock.unlock();
        barrier2.wait();
        return ret;
      }
    };

#endif
  }
}
#endif

