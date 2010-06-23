#ifndef __HASH_HPP__
#define __HASH_HPP__

#include <iostream>
#include <utility>

namespace jellyfish {
  /* Wrapper around a "storage". The hash class manages threads. In
     particular, it synchronizes the threads for the size-doubling
     operation. The storage class is reponsible for the details of
     storing the key,value pairs, memory management, reprobing, etc.
   */

  template<typename key_t, typename val_t, typename storage_t, typename atomic_t>
  class hash {
  public:
    typedef typename std::pair<key_t,val_t> kv_t;
    typedef typename storage_t::iterator iterator;

    class TableFull : public std::exception {
      virtual const char* what() const throw() {
        return "Mapped region is invalid";
      }
    };
    
    hash(storage_t *_ary) : ary(_ary) { }
    hash(char *map, size_t length) { ary = new storage_t(map, length); }

    size_t get_size() { return ary->get_size(); }
    uint_t get_key_len() { return ary->get_key_len(); }
    uint_t get_val_len() { return ary->get_val_len(); }

    size_t get_max_reprobe_offset() { return ary->get_max_reprobe_offset(); }

    void write_raw(std::ostream &out) { ary->write_raw(out); }

    iterator iterator_all() { return ary->iterator_all(); }
    iterator iterator_slice(size_t slice_number, size_t number_of_slice) {
      return ary->iterator_slice(slice_number, number_of_slice);
    }

    /*
     * Thread handle to the hash. May implement size doubling of
     * hash. Does special handling of zero key.
     */
    class thread {
      storage_t *ary;
      size_t     hsize_mask;
    
    public:
      thread(storage_t *_ary, size_t _size) :
        ary(_ary), hsize_mask(_size - 1) {}

      inline void add(key_t key, val_t val) {
        // If size doubling enabled, should double size instead of
        // throwing an error.
        if(!ary->add(key, val))
          throw new TableFull();
      }

      inline void inc(key_t key) { return this->add(key, (val_t)1); }
    };
    thread *new_hash_counter() { return new thread(ary, ary->get_size()); }

    //private:
  public:
    storage_t *ary;
  };
}

#endif // __HASH_HPP__
