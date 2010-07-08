#ifndef __HASH_HPP__
#define __HASH_HPP__

#include <iostream>
#include <fstream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "lib/misc.hpp"
#include "lib/locks_pthread.hpp"
#include "lib/square_binary_matrix.hpp"
#include "compacted_hash.hpp"


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
    
    hash(uint_t _threads) : 
      worker_threads(_threads), file_prefix(NULL), file_index(0), ary(NULL),
      out_buffer_size(20000000), out_val_len(4) { }
    hash(uint_t _threads, storage_t *_ary) : 
      worker_threads(_threads), file_prefix(NULL), file_index(0), ary(_ary),
      out_buffer_size(20000000), out_val_len(4) { }
    hash(char *map, size_t length) :
      worker_threads(0), file_prefix(NULL), file_index(0)
    {
      ary = new storage_t(map, length);
    }
    hash(const char *filename, bool sequential) :
      worker_threads(0), file_prefix(NULL), file_index(0) {
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

    void set_out_val_len(uint64_t vlen) { out_val_len = vlen; }
    void set_out_buffer_size(size_t size) { out_buffer_size = size; }
    void set_file_prefix(const char *new_prefix) {
      if(file_prefix)
	delete [] file_prefix;
      file_prefix = new char[strlen(new_prefix)+1];
      strcpy(file_prefix, new_prefix);
    }

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
      // struct header {
      // 	uint64_t  key_len;
      // 	uint64_t  val_len; // In bytes
      // 	uint64_t  size; // In bytes
      // 	uint64_t  max_reprobes;
      // 	uint64_t  unique;
      // 	uint64_t  distinct;
      // 	uint64_t  total;
      // };
      
      storage_t			*ary;
      uint_t			 threads;
      std::ostream		*out;
      uint64_t			 key_len_bits;
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

      dump_zero() : buffers(NULL) { }

      void init(storage_t *_ary, std::ostream *os, uint_t _threads,
                size_t _buffer_size, uint_t klen, uint_t vlen) {
        ary           = _ary;
        out           = os;
        threads       = _threads;
        key_len_bits  = klen;
        val_len_bits  = vlen;
        key_len       = (klen / 8) + (klen % 8 != 0);
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
	  delete[] buffers;
	  delete[] thread_info;
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
	jellyfish::compacted_hash::header head;

	memset(&head, '\0', sizeof(head));
	head.key_len = key_len_bits;
	head.val_len = val_len;
	head.size = ary->get_size();
	head.max_reprobes = ary->get_max_reprobe_offset();
	out->write((char *)&head, sizeof(head));
	ary->write_ary_header(*out);
	return out->good();
      }

      bool update_stats() {
	struct header head;
	head.key_len = key_len_bits;
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
    enum status_t { FREE, INUSE, BLOCKED };
    class thread {
      storage_t			*ary;
      size_t			 hsize_mask;
      typename atomic_t::type	 status;
      typename atomic_t::type	 ostatus;
      hash			*my_hash;
      atomic_t			 atomic;
    
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
	    my_hash->dump_to_file();
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

    /**
     * Actual dumping of hash to file and zeroing of memory. Called by
     * thread triggering the event after it secured all the necessary
     * locks.
     **/
    void dump_to_file() {
      static const long file_len = pathconf("/", _PC_PATH_MAX);
      if(!worker_threads || !file_prefix)
	throw new TableFull();

      char file[file_len + 1];
      file[file_len] = '\0';
      int off = snprintf(file, file_len, "%s", file_prefix);
      if(off > 0 && off < file_len)
	off += snprintf(file + off, file_len - off, "_%" PRIUINTu, file_index++);
      if(off < 0 || off >= file_len)
	throw new TableFull();
      
      std::ofstream out(file);
      if(out.fail())
	throw new TableFull();

      // TODO: parameters should be given passed to hash
      dump_zero dumper = new_dumper(&out, worker_threads, out_buffer_size,
				    get_key_len(), 8 * out_val_len);
      struct dump_to_file_info infos[worker_threads];
      for(uint_t i = 0; i < worker_threads; i++) {
	infos[i].id = i;
	infos[i].dumper = &dumper;
	pthread_create(&infos[i].thread_id, NULL, dump_to_file_thread, &infos[i]);
      }

      for(uint_t i = 0; i < worker_threads; i++)
	pthread_join(infos[i].thread_id, NULL);
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

    struct dump_to_file_info {
      uint_t	 id;
      dump_zero *dumper;
      pthread_t  thread_id;
    };
    static void *dump_to_file_thread(void *arg) {
      struct dump_to_file_info *info = (struct dump_to_file_info *)arg;
      info->dumper->dump(info->id);
      return NULL;
    }

  private:
    //  public:
    uint_t			 worker_threads;
    char			*file_prefix;
    uint_t			 file_index;
    storage_t			*ary;
    thread_list_t		 user_thread_list;
    locks::pthread::mutex	 user_thread_lock;
    locks::pthread::mutex	 event_lock;
    locks::pthread::cond	 inuse_thread_cond;
    volatile uint_t		 inuse_thread_count;
    size_t			 out_buffer_size;
    uint64_t			 out_val_len;
    atomic_t			 atomic;
  };
}

#endif // __HASH_HPP__
