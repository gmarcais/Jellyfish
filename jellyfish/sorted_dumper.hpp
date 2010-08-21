#include "dumper.hpp"
#include "heap.hpp"
#include "time.hpp"

namespace jellyfish {
  template<typename storage_t, typename atomic_t>
  class sorted_dumper : public dumper_t {
    typedef typename storage_t::overlap_iterator iterator;
    typedef compacted_hash::writer<storage_t> writer_t;
    typedef heap_t<iterator> oheap_t;
    struct thread_info_t {
      pthread_t             thread_id;
      uint_t                id;
      locks::pthread::cond  cond;
      volatile bool         token;
      writer_t              writer;
      oheap_t                heap;
      sorted_dumper        *self;
    };

    uint_t                threads;
    std::string           file_prefix;
    size_t                buffer_size;
    uint_t                klen, vlen;
    uint_t                key_len, val_len;
    size_t                record_len, nb_records, nb_blocks;
    storage_t            *ary;
    int                   file_index;
    Time                  writing_time;
    struct thread_info_t *thread_info;
    uint64_t              max_count;
    uint64_t volatile     unique, distinct, total;
    std::ofstream         out;

  public:
    // klen: key field length in bits in hash (i.e before rounding up to bytes)
    // vlen: value field length in bits
    sorted_dumper(uint_t _threads, const char *_file_prefix, size_t _buffer_size, 
                     uint_t _vlen, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix), buffer_size(_buffer_size),
      klen(_ary->get_key_len()), vlen(_vlen), ary(_ary), file_index(0),
      writing_time(Time::zero)
    {
      key_len    = bits_to_bytes(klen);
      val_len    = bits_to_bytes(vlen);
      max_count  = (((uint64_t)1) << (8*val_len)) - 1;
      record_len = key_len + val_len;
      nb_records = ary->floor_block(_buffer_size / record_len, nb_blocks);
      while(nb_records < ary->get_max_reprobe_offset()) {
        nb_records = ary->floor_block(2 * nb_records, nb_blocks);
      }

      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        thread_info[i].token = i == 0;
        thread_info[i].writer.initialize(nb_records, klen, vlen, ary);
        thread_info[i].heap.initialize(ary->get_max_reprobe_offset());
        thread_info[i].id = i;
        thread_info[i].self = this;
      }
      unique = distinct = total = 0;
    }

    ~sorted_dumper() {
      if(thread_info) {
        delete[] thread_info;
      }
    }

    Time get_writing_time() const { return writing_time; }

    static void *dump_to_file_thread(void *arg) {
      struct thread_info_t *info = (struct thread_info_t *)arg;
      info->self->dump_to_file(info);
      return NULL;
    }

    void dump_to_file(struct thread_info_t *my_info);

    virtual void dump();
    void update_stats() {
      thread_info[0].writer.update_stats_with(&out, unique, distinct, total);
    }
  };

  template<typename storage_t, typename atomic_t>
  void sorted_dumper<storage_t,atomic_t>::dump() {
    static const long file_len = pathconf("/", _PC_PATH_MAX);

    Time start_dump;
    char file[file_len + 1];
    file[file_len] = '\0';
    int off = snprintf(file, file_len, "%s", file_prefix.c_str());
    if(off > 0 && off < file_len)
      off += snprintf(file + off, file_len - off, "_%d", file_index++);
      //off += snprintf(file + off, file_len - off, "_%" PRIUINTu, file_index++);
    if(off < 0 || off >= file_len)
      return; // TODO: Should throw an error
      
    out.open(file);
    if(out.fail())
      return; // TODO: Should throw an error

    unique = distinct = total = 0;
    for(uint_t i = 0; i < threads; i++)
      thread_info[i].token = i == 0;
    for(uint_t i = 0; i < threads; i++) {
      pthread_create(&thread_info[i].thread_id, NULL, dump_to_file_thread, 
                     &thread_info[i]);
    }

    for(uint_t i = 0; i < threads; i++)
      pthread_join(thread_info[i].thread_id, NULL);
    ary->zero_blocks(0, nb_blocks); // zero out last group of blocks
    update_stats();
    out.close();

    Time end_dump;
    writing_time += end_dump - start_dump;
  }

  template<typename storage_t, typename atomic_t>
  void sorted_dumper<storage_t,atomic_t>::dump_to_file(struct thread_info_t *my_info) {
    size_t                i;
    struct thread_info_t *next_info = &thread_info[(my_info->id + 1) % threads];
    atomic_t              atomic;

    if(my_info->token && my_info->id == 0)
      my_info->writer.write_header(&out);


    size_t append = 0;
    for(i = my_info->id; i * nb_records < ary->get_size(); i += threads) {
      // fill up buffer
      iterator it(ary, i * nb_records, (i + 1) * nb_records);
      my_info->heap.fill(it);

      while(it.next()) {
        typename oheap_t::const_item_t item = my_info->heap.head();
        my_info->writer.append(item->key, item->val);
        append++;
        my_info->heap.pop();
        my_info->heap.push(it);
      }

      while(my_info->heap.is_not_empty()) {
        typename oheap_t::const_item_t item = my_info->heap.head();
        my_info->writer.append(item->key, item->val);
        append++;
        my_info->heap.pop();
      }

      // wait for token & write buffer
      my_info->cond.lock();
      while(!my_info->token) { my_info->cond.wait(); }
      my_info->cond.unlock();
      my_info->writer.dump(&out);
          
      // pass on token
      my_info->token = false;
      next_info->cond.lock();
      next_info->token = true;
      next_info->cond.signal();
      next_info->cond.unlock();

      // zero out memory
      if(i > 0)
        ary->zero_blocks(i * nb_blocks, nb_blocks);
    }

    atomic.add_fetch(&unique, my_info->writer.get_unique());
    atomic.add_fetch(&distinct, my_info->writer.get_distinct());
    atomic.add_fetch(&total, my_info->writer.get_total());
  }
}
