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
#include <jellyfish/err.hpp>
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

  template<typename key_t, typename val_t, typename ary_t, typename atomic>
  class hash : public hash_t {
  public:
    define_error_class(TableFull);
    //    typedef typename std::pair<key_t,val_t> kv_t;
    typedef ary_t storage_t;
    typedef typename ary_t::iterator iterator;

    hash() : ary(NULL), dumper(NULL), dumping_initiated(false) {}
    explicit hash(ary_t *_ary) : ary(_ary), dumper(NULL), dumping_initiated(false) {}

    virtual ~hash() {}

    size_t get_size() const { return ary->get_size(); }
    uint_t get_key_len() const { return ary->get_key_len(); }
    uint_t get_val_len() const { return ary->get_val_len(); }
    uint_t get_max_reprobe() const { return ary->get_max_reprobe(); }
    size_t get_max_reprobe_offset() const { return ary->get_max_reprobe_offset(); }
    
    void set_dumper(dumper_t *new_dumper) { dumper = new_dumper; }
    Time get_writing_time() const { 
      if(!dumper)
        return Time::zero;
      return dumper->get_writing_time();
    }

    void write_raw(std::ostream &out) { ary->write_raw(out); }

    iterator iterator_all() const { return ary->iterator_all(); }
    iterator iterator_slice(size_t slice_number, size_t number_of_slice) const {
      return ary->iterator_slice(slice_number, number_of_slice);
    }

    /*
     * Thread handle to the hash.
     */
    enum status_t { FREE, INUSE, BLOCKED };
    class thread {
      ary_t    *ary;
      size_t    hsize_mask;
      status_t  status;
      status_t  ostatus;
      hash     *my_hash;
    
    public:
      typedef val_t val_type;
      thread(ary_t *_ary, hash *_my_hash) :
        ary(_ary), hsize_mask(ary->get_size() - 1), status(FREE), my_hash(_my_hash)
      { }

      // Add val to the value associated with key. Returns the old
      // value in *oval if oval is not NULL
      template<typename add_t>
      inline void add(key_t key, const add_t &val, val_t *oval = 0) {
        while(true) {
          while(atomic::cas(&status, FREE, INUSE) != FREE)
            my_hash->wait_event_is_done();

          if(ary->add(key, val, oval))
            break;

          // Improve this. Undefined behavior if dump_to_file throws an error.
          if(my_hash->get_event_locks()) {
            my_hash->dump();
            my_hash->release_event_locks();
          }
        }
        
        if(atomic::cas(&status, INUSE, FREE) != INUSE)
          my_hash->signal_not_in_use();
      }

      // void inc(key_t key, val_t *oval = 0) { return this->add(key, (add_t)1, oval); }
      // inline void operator()(key_t key) { return this->add(key, (val_t)1); }

      friend class hash;
    };
    friend class thread;
    typedef std::list<thread> thread_list_t;
    class thread_ptr_t : public thread_list_t::iterator {
    public:
      explicit thread_ptr_t(const typename thread_list_t::iterator &thl) : thread_list_t::iterator(thl) {}
      typedef val_t val_type;
    };
    // typedef typename thread_list_t::iterator thread_ptr_t;

    thread_ptr_t new_thread() { 
      user_thread_lock.lock();
      thread_ptr_t res(user_thread_list.insert(user_thread_list.begin(), thread(ary, this)));
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
        eraise(TableFull) << "No dumper defined";
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
    void signal_not_in_use(bool take_inuse_lock = true) {
      if(take_inuse_lock)
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
    void wait_event_is_done(bool take_event_lock = true) {
      if(take_event_lock)
        event_cond.lock();
      while(dumping_initiated)
        event_cond.wait();
      event_cond.unlock();
    }

    /**
     * Get the locks before handling an event and returns true if
     * success. It guarantees than no thread is doing an operation on
     * the hash. If another thread is already handling an event,
     * degrade to wait_event_is_done and returns false.
     **/
    bool get_event_locks() {
      inuse_thread_cond.lock();
      event_cond.lock();
      if(dumping_initiated) {
        // Another thread is doing the dumping
        signal_not_in_use(false);
        wait_event_is_done(false);
        return false;
      }

      // I am the thread doing the dumping
      user_thread_lock.lock();
      dumping_initiated = true;
      event_cond.unlock();

      inuse_thread_count = 0;
      
      // Block access to hash and wait for threads with INUSE state
      for(thread_ptr_t it(user_thread_list.begin());
          it != user_thread_list.end();
          it++) {
        it->ostatus = atomic::set(&it->status, BLOCKED);
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
      event_cond.lock();
      for(thread_ptr_t it(user_thread_list.begin());
          it != user_thread_list.end();
          it++) {
        atomic::set(&it->status, FREE);
      }
      user_thread_lock.unlock();
      dumping_initiated = false;
      event_cond.broadcast();
      event_cond.unlock();
    }

  private:
    ary_t                 *ary;
    dumper_t              *dumper;
    volatile bool          dumping_initiated;
    thread_list_t          user_thread_list;
    locks::pthread::mutex  user_thread_lock;
    locks::pthread::cond   event_cond;
    locks::pthread::cond   inuse_thread_cond;
    volatile uint_t        inuse_thread_count;
  };
}

#endif // __HASH_HPP__
