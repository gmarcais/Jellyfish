#ifndef __HASH_HPP__
#define __HASH_HPP__

#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "lib/misc.hpp"
#include "lib/locks_pthread.hpp"
#include "lib/square_binary_matrix.hpp"

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
        return "Table is full";
      }
    };
    class MappingError : public StandardError { };
    
    hash() : ary(NULL) { }
    hash(storage_t *_ary) : ary(_ary) { }
    hash(char *map, size_t length) {
      std::cerr << "map " << (void *)map << " length " << length << std::endl;
      ary = new storage_t(map, length);
    }
    hash(const char *filename, bool sequential) {
      std::cerr << "from file " << filename << std::endl;
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

    size_t get_size() { return ary->get_size(); }
    uint_t get_key_len() { return ary->get_key_len(); }
    uint_t get_val_len() { return ary->get_val_len(); }

    size_t get_max_reprobe_offset() { return ary->get_max_reprobe_offset(); }

    void write_raw(std::ostream &out) { ary->write_raw(out); }

    iterator iterator_all() { return ary->iterator_all(); }
    iterator iterator_slice(size_t slice_number, size_t number_of_slice) {
      return ary->iterator_slice(slice_number, number_of_slice);
    }

    /**
     * The constructor needs to be called with the number of threads
     * t. Each thread must call the dump method with an ID between 0
     * and t-1.
     **/
    class dump_zero {
      struct thread_info_t {
        locks::pthread::cond  cond;
        volatile bool         token;
        char                 *buffer;
      };
      struct header {
	uint64_t  mer_len;
	uint64_t  val_len; // In bytes
	uint64_t  size; // In bytes
	uint64_t  unique;
	uint64_t  distinct;
	uint64_t  total;
      };

      storage_t			*ary;
      uint_t			 threads;
      std::ostream		*out;
      uint64_t			 mer_len_bases;
      uint64_t			 val_len_bits;
      uint64_t			 key_len;	// in BYTES
      uint64_t			 val_len;	// in BYTES
      uint64_t			 max_count;
      char			*buffers;
      struct thread_info_t	*thread_info;
      size_t			 record_len, nb_records, nb_blocks;
      uint64_t volatile          unique, distinct, total;

    public:
      dump_zero(storage_t *_ary, std::ostream *os, uint_t _threads,
                size_t _buffer_size, uint_t klen, uint_t vlen)
      {
        init(_ary, os, _threads, _buffer_size, klen, vlen);
      }

      dump_zero() : buffers(NULL) {}

      void init(storage_t *_ary, std::ostream *os, uint_t _threads,
                size_t _buffer_size, uint_t klen, uint_t vlen) {
        ary           = _ary;
        out           = os;
        threads       = _threads;
        mer_len_bases = klen;
        val_len_bits  = vlen;
        key_len       = (klen / 4) + (klen % 4 != 0);
        val_len       = (vlen / 8) + (vlen % 8 != 0);
        max_count     = (((uint64_t)1) << (8*val_len)) - 1;
        record_len    = key_len + val_len;
        nb_records    = ary->floor_block(_buffer_size / record_len, nb_blocks);

        buffers = new char[nb_records * record_len * threads];
        thread_info = new struct thread_info_t[threads];
        for(uint_t i = 0; i < threads; i++) {
          thread_info[i].token = i == 0;
          thread_info[i].buffer = &buffers[i * nb_records * record_len];
        }

	unique = distinct = total = 0;
      }
      
      ~dump_zero() {
	if(buffers) {
	  //	  delete[] buffers; Why does this bomb?
	  buffers = NULL;
	}
      }

      void dump(uint_t id) {
        size_t			 i;
	uint64_t		 count;
        struct thread_info_t	*my_info   = &thread_info[id];
	struct thread_info_t	*next_info = &thread_info[(id + 1) % threads];
	uint64_t		 _unique = 0;
	uint64_t                 _distinct = 0;
	uint64_t                 _total = 0;
	atomic_t		 atomic;

	if(my_info->token)
	  write_header();

	for(i = id; i * nb_records < ary->get_size(); i += threads) {
	  // fill up buffer
          iterator it(ary, i * nb_records, (i + 1) * nb_records);
          char *ptr = my_info->buffer;

          while(it.next()) {
            memcpy(ptr, &it.key, key_len);
            ptr += key_len;
            count = (it.val > max_count) ? max_count : it.val;
            memcpy(ptr, &count, val_len);
            ptr += val_len;
	    _unique += it.val == 1;
	    _distinct++;
	    _total += it.val;
          }

          my_info->cond.lock();
          while(!my_info->token) { my_info->cond.wait(); }
          my_info->cond.unlock();
	  out->write((char *)my_info->buffer, ptr - my_info->buffer);
          
	  // wait for token & write buffer
	  // pass on token
          my_info->token = false;
          next_info->cond.lock();
          next_info->token = true;
          next_info->cond.signal();
          next_info->cond.unlock();

	  // zero out memory
	  ary->zero_blocks(i * nb_blocks, nb_blocks);
        }
	atomic.add_fetch(&unique, _unique);
	atomic.add_fetch(&distinct, _distinct);
	atomic.add_fetch(&total, _total);
      }

      bool write_header() {
	struct header head;

	memset(&head, '\0', sizeof(head));
	head.mer_len = mer_len_bases;
	head.val_len = val_len;
	out->write((char *)&head, sizeof(header));
	ary->write_hash_matrices(*out);
	return out->good();
      }

      bool update_stats() {
	struct header head;
	head.mer_len = mer_len_bases;
	head.val_len = val_len;
	head.unique = unique;
	head.distinct = distinct;
	head.total = total;
	out->seekp(0);
	out->write((char *)&head, sizeof(head));
	return out->good();
      }
    };
    friend class dump_zero;
    // We already know something about threads who registered with
    // new_hash_counter(). Should we use it here?
    // dump_zero new_dumper(std::string filename, uint_t _threads,
    // 			 size_t _buffer_size, uint_t klen, uint_t vlen) { 
    //   return dump_zero(this, filename, _threads, _buffer_size, klen, vlen);
    // }
    dump_zero new_dumper(std::ostream *out, uint_t _threads,
			 size_t _buffer_size, uint_t klen, uint_t vlen) { 
      return dump_zero(ary, out, _threads, _buffer_size, klen, vlen);
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
