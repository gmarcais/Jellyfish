#include "dumper.hpp"
#include <algorithm>

namespace jellyfish {
  template<typename storage_t, typename atomic_t>
  class sorted_dumper : public dumper_t {
    typedef typename storage_t::overlap_iterator iterator;
    typedef compacted_hash::writer<storage_t> writer_t;
    struct thread_info_t {
      pthread_t             thread_id;
      uint_t                id;
      locks::pthread::cond  cond;
      volatile bool         token;
      writer_t              writer;
      sorted_dumper     *self;
    };

    uint_t                threads;
    std::string           file_prefix;
    size_t                buffer_size;
    uint_t                klen, vlen;
    uint_t                key_len, val_len;
    size_t                record_len, nb_records, nb_blocks;
    storage_t            *ary;
    uint_t                file_index;
    struct thread_info_t *thread_info;
    uint64_t              max_count;
    uint64_t volatile     unique, distinct, total;
    std::ofstream         out;

    struct heap_item {
      uint64_t       key;
      uint64_t       val;
      uint64_t       pos;

      heap_item() : key(0), val(0), pos(0) { }

      heap_item(iterator &iter) { 
        initialize(iter);
      }

      void initialize(iterator &iter) {
        key = iter.key;
        val = iter.val;
        pos = iter.get_pos();
      }

      bool operator<(const heap_item & other) {
        if(pos == other.pos)
          return key > other.key;
        return pos > other.pos;
      }
    };

    class heap_item_compare {
    public:
      inline bool operator() (struct heap_item *item1, struct heap_item *item2) {
        return *item1 < *item2;
      }
    };

  public:
    // klen: key field length in bits in hash (i.e before rounding up to bytes)
    // vlen: value field length in bits
    sorted_dumper(uint_t _threads, const char *_file_prefix, size_t _buffer_size, 
                     uint_t _vlen, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix), buffer_size(_buffer_size),
      klen(_ary->get_key_len()), vlen(_vlen), ary(_ary), file_index(0)
    {
      key_len    = bits_to_bytes(klen);
      val_len    = bits_to_bytes(vlen);
      max_count  = (((uint64_t)1) << (8*val_len)) - 1;
      record_len = key_len + val_len;
      nb_records = ary->floor_block(_buffer_size / record_len, nb_blocks);
      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        thread_info[i].token = i == 0;
        thread_info[i].writer.initialize(nb_records, klen, vlen, ary);
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

    static void *dump_to_file_thread(void *arg) {
      struct thread_info_t *info = (struct thread_info_t *)arg;
      info->self->dump_to_file(info);
      return NULL;
    }

    void dump_to_file(struct thread_info_t *my_info);

    virtual void dump();
    void update_stats() {
      thread_info[0].writer.update_stats(&out, unique, distinct, total);
    }
  };

  template<typename storage_t, typename atomic_t>
  void sorted_dumper<storage_t,atomic_t>::dump() {
    static const long file_len = pathconf("/", _PC_PATH_MAX);

    char file[file_len + 1];
    file[file_len] = '\0';
    int off = snprintf(file, file_len, "%s", file_prefix.c_str());
    if(off > 0 && off < file_len)
      off += snprintf(file + off, file_len - off, "_%" PRIUINTu, file_index++);
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
  }

  template<typename storage_t, typename atomic_t>
  void sorted_dumper<storage_t,atomic_t>::dump_to_file(struct thread_info_t *my_info) {
    size_t                i;
    struct thread_info_t *next_info = &thread_info[(my_info->id + 1) % threads];
    atomic_t              atomic;
    heap_item             heap_storage[ary->get_max_reprobe_offset()];
    heap_item            *heap[ary->get_max_reprobe_offset()];
    heap_item_compare     compare;

    if(my_info->token && my_info->id == 0)
      my_info->writer.write_header(&out);

    for(i = my_info->id; i * nb_records < ary->get_size(); i += threads) {
      // fill up buffer
      iterator it(ary, i * nb_records, (i + 1) * nb_records);
      size_t h = 0;
      for(h = 0; h < ary->get_max_reprobe_offset() && it.next(); h++) {
        heap_storage[h].initialize(it);
        heap[h] = &heap_storage[h];
      }
      make_heap(heap, heap + h, compare);

      while(it.next()) {
        pop_heap(heap, heap + h--, compare);
        my_info->writer.append(heap[h]->key, heap[h]->val);
        heap[h]->initialize(it);
        push_heap(heap, heap + ++h, compare);
      }

      while(h > 0) {
        pop_heap(heap, heap + h--, compare);
        my_info->writer.append(heap[h]->key, heap[h]->val);
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
