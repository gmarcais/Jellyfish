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

#ifndef __JELLYFISH_HASH_HPP__
#define __JELLYFISH_HASH_HPP__

#include <iostream>
#include <fstream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <jellyfish/mapped_file.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/dumper.hpp>
#include <jellyfish/time.hpp>

namespace jellyfish {
  /* Wrapper around a "storage". The hash class manages threads. In
     particular, it synchronizes the threads for the size-doubling
     operation and manages dumping the hash to disk. The storage class
     is reponsible for the details of storing the key,value pairs,
     memory management, reprobing, etc.
  */

  class hash_t {
  public:
    virtual ~hash_t() {}
  };

  template<typename key_t, typename val_t, typename ary_t, typename atomic_t>
  class hash : public hash_t {
  public:
    define_error_class(TableFull);
    typedef typename std::pair<key_t,val_t> kv_t;
    typedef ary_t storage_t;
    typedef typename ary_t::iterator iterator;

    hash() : ary(NULL), dumper(NULL) {}
    hash(ary_t *_ary) : ary(_ary), dumper(NULL) {}
    hash(char *map, size_t length) : 
      ary(new ary_t(map, length)),
      dumper(NULL) { }
    hash(const char *filename, bool sequential) {
      dumper = NULL;
      open(filename, sequential);
    }

    virtual ~hash() {}

    void open(const char *filename, bool sequential) {
      mapped_file mf(filename);
      if(sequential)
        mf.sequential();
      ary = new ary_t(mf.base(), mf.length());
    }

    size_t get_size() const { return ary->get_size(); }
    uint_t get_key_len() const { return ary->get_key_len(); }
    uint_t get_val_len() const { return ary->get_val_len(); }
    size_t get_max_reprobe_offset() const { return ary->get_max_reprobe_offset(); }
    
    void set_dumper(dumper_t *new_dumper) { dumper = new_dumper; }
    Time get_writing_time() const { 
      if(!dumper)
        return Time::zero;
      return dumper->get_writing_time();
    }

    void write_raw(std::ostream &out) { ary->write_raw(out); }

    iterator iterator_all() { return ary->iterator_all(); }
    iterator iterator_slice(size_t slice_number, size_t number_of_slice) {
      return ary->iterator_slice(slice_number, number_of_slice);
    }

    /*
     * Thread handle to the hash.
     */
    enum status_t { FREE, INUSE, BLOCKED };
    class thread {
      ary_t                 *ary;
      size_t                     hsize_mask;
      typename atomic_t::type    status;
      typename atomic_t::type    ostatus;
      hash                      *my_hash;
      atomic_t                   atomic;
    
    public:
      thread(ary_t *_ary, hash *_my_hash) :
        ary(_ary), hsize_mask(ary->get_size() - 1), status(FREE), my_hash(_my_hash)
      { }

      inline void add(key_t key, val_t val) {
        while(true) {
          while(atomic.cas(&status, FREE, INUSE) != FREE)
            my_hash->wait_event_is_done();

          if(ary->add(key, val))
            break;

          // Improve this. Undefined behavior if dump_to_file throws an error.
          if(my_hash->get_event_locks()) {
            my_hash->dump();
            my_hash->release_event_locks();
          }
        }
        
        if(atomic.cas(&status, INUSE, FREE) != INUSE)
          my_hash->signal_not_in_use();
      }

      inline void inc(key_t key) { return this->add(key, (val_t)1); }
      inline void operator()(key_t key) { return this->add(key, (val_t)1); }

      friend class hash;
    };
    friend class thread;
    typedef std::list<thread> thread_list_t;
    typedef typename thread_list_t::iterator thread_ptr_t;

    thread_ptr_t new_thread() { 
      user_thread_lock.lock();
      thread_ptr_t res = 
        user_thread_list.insert(user_thread_list.begin(), thread(ary, this));
      user_thread_lock.unlock();
      return res;
    }

    void release_thread(thread_ptr_t &th) {
      user_thread_lock.lock();
      user_thread_list.erase(th);
      user_thread_lock.unlock();
    }

    void dump() {
      if(dumper)
        dumper->dump();
      else
        throw_error<TableFull>("No dumper defined");
    }

  private:
    /**
     * The following methods are called by threads to manage
     * administrative events: size doubling or dumping the hash to
     * disk.
     **/

    /**
     * Called by a thread if it failed to switch its states from INUSE
     * to FREE. It lets the thread triggering the event that the hash
     * is free. This method returns after the signaling and does not
     * wait for the handling of the event to be over.
     **/
    void signal_not_in_use() {
      inuse_thread_cond.lock();
      if(--inuse_thread_count == 0)
        inuse_thread_cond.signal();
      inuse_thread_cond.unlock();
    }

    /**
     * Called by a thread if it failed to switch its states from FREE
     * to INUSE. An event management has been initiated. This call
     * waits for the event handling to be over.
     **/
    void wait_event_is_done() {
      event_lock.lock();
      event_lock.unlock();
    }

    /**
     * Get the locks before handling an event and returns true if
     * success. It guarantees than no thread is doing an operation on
     * the hash. If another thread is already handling an event,
     * degrade to wait_event_is_done and returns false.
     **/
    bool get_event_locks() {
      inuse_thread_cond.lock();
      if(!event_lock.try_lock()) {
        inuse_thread_cond.unlock();
        signal_not_in_use();
        wait_event_is_done();
        return false;
      }
      user_thread_lock.lock();
      inuse_thread_count = 0;
      
      // Block access to hash and wait for threads with INUSE state
      for(thread_ptr_t it = user_thread_list.begin(); 
          it != user_thread_list.end();
          it++) {
        it->ostatus = atomic.set(&it->status, BLOCKED);
        if(it->ostatus == INUSE)
          inuse_thread_count++;
      }
      inuse_thread_count--; // Remove 1 for myself!
      while(inuse_thread_count > 0) {
        inuse_thread_cond.wait();
      }
      inuse_thread_cond.unlock();

      return true;
    }    

    void release_event_locks() {
      for(thread_ptr_t it = user_thread_list.begin(); 
          it != user_thread_list.end();
          it++) {
        atomic.set(&it->status, FREE);
      }
      user_thread_lock.unlock();
      event_lock.unlock();
    }

  private:
    ary_t                 *ary;
    dumper_t              *dumper;
    thread_list_t          user_thread_list;
    locks::pthread::mutex  user_thread_lock;
    locks::pthread::mutex  event_lock;
    locks::pthread::cond   inuse_thread_cond;
    volatile uint_t        inuse_thread_count;
    atomic_t               atomic;
  };
}

#endif // __HASH_HPP__
