#ifndef __HASH_HPP__
#define __HASH_HPP__

#include <iostream>
#include <fstream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "locks_pthread.hpp"
#include "misc.hpp"
#include "square_binary_matrix.hpp"
#include "dumper.hpp"


namespace jellyfish {
  /* Wrapper around a "storage". The hash class manages threads. In
     particular, it synchronizes the threads for the size-doubling
     operation and manages dumping the hash to disk. The storage class
     is reponsible for the details of storing the key,value pairs,
     memory management, reprobing, etc.
   */

  template<typename key_t, typename val_t, typename storage_t, typename atomic_t>
  class hash {
  public:
    typedef typename std::pair<key_t,val_t> kv_t;
    typedef typename storage_t::iterator iterator;

    class TableFull : public std::exception {
      virtual const char* what() const throw() {
        return "Table is full";
      }
    };
    class MappingError : public StandardError { };
    
    hash() : ary(NULL) {}
    hash(storage_t *_ary) : ary(_ary) {}
    hash(char *map, size_t length) {
      ary = new storage_t(map, length);
    }
    hash(const char *filename, bool sequential) {
      open(filename, sequential);
    }

    void open(const char *filename, bool sequential) {
      struct stat finfo;
      char *map;
      int fd = ::open(filename, O_RDONLY);

      std::cerr << "opening " << filename << " sequential " << sequential << std::endl;
      if(fd < 0)
        throw new StandardError(errno, "Can't open file '%s'", filename);
      if(fstat(fd, &finfo) < 0)
        throw new StandardError(errno, "Can't stat '%s'",
                                filename);
      map = (char *)mmap(NULL, finfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
      if(map == MAP_FAILED)
        throw new StandardError(errno, "Can't mmap '%s'", filename);
      if(sequential) {
        int adv = madvise(map, finfo.st_size, MADV_SEQUENTIAL);
        if (adv != 0)
          throw new StandardError(errno, "Can't set memory parameters");
      }
      close(fd);
      ary = new storage_t(map, finfo.st_size);
    }

    size_t get_size() const { return ary->get_size(); }
    uint_t get_key_len() const { return ary->get_key_len(); }
    uint_t get_val_len() const { return ary->get_val_len(); }
    size_t get_max_reprobe_offset() const { return ary->get_max_reprobe_offset(); }
    
    void set_dumper(dumper_t *new_dumper) { dumper = new_dumper; }

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
      storage_t                 *ary;
      size_t                     hsize_mask;
      typename atomic_t::type    status;
      typename atomic_t::type    ostatus;
      hash                      *my_hash;
      atomic_t                   atomic;
    
    public:
      thread(storage_t *_ary, hash *_my_hash) :
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

      friend class hash;
    };
    friend class thread;
    typedef list<thread> thread_list_t;
    typedef typename thread_list_t::iterator thread_ptr_t;

    thread_ptr_t new_hash_counter() { 
      user_thread_lock.lock();
      thread_ptr_t res = 
        user_thread_list.insert(user_thread_list.begin(), thread(ary, this));
      user_thread_lock.unlock();
      return res;
    }

    void relase_hash_counter(thread_ptr_t &th) {
      user_thread_lock.lock();
      user_thread_list.erase(th);
      user_thread_lock.unlock();
    }

    void dump() {
      if(dumper)
        dumper->dump();
      // TODO: should throw an error if dumper == NULL ?
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
    storage_t             *ary;
    thread_list_t          user_thread_list;
    locks::pthread::mutex  user_thread_lock;
    locks::pthread::mutex  event_lock;
    locks::pthread::cond   inuse_thread_cond;
    volatile uint_t        inuse_thread_count;
    dumper_t              *dumper;
    atomic_t               atomic;
  };
}

#endif // __HASH_HPP__
