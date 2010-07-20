#include "dumper.hpp"

namespace jellyfish {
  template<typename storage_t, typename atomic_t>
  class compacted_dumper : public dumper_t {
    typedef typename storage_t::iterator iterator;
    struct thread_info_t {
      pthread_t             thread_id;
      uint_t                id;
      locks::pthread::cond  cond;
      volatile bool         token;
      char                 *buffer;
      compacted_dumper     *self;
    };

    uint_t                threads;
    std::string           file_prefix;
    size_t                buffer_size;
    uint_t                klen, vlen;
    storage_t            *ary;
    uint_t                file_index;
    uint_t                key_len, val_len;
    char                 *buffers;
    struct thread_info_t *thread_info;
    size_t                record_len, nb_records, nb_blocks;
    uint64_t              max_count;
    uint64_t volatile     unique, distinct, total;
    std::ofstream         out;

  public:
    // klen: key field length in bits in hash (i.e before rounding up to bytes)
    // vlen: value field length in bits
    compacted_dumper(uint_t _threads, const char *_file_prefix, size_t _buffer_size, 
                     uint_t _vlen, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix), buffer_size(_buffer_size),
      klen(_ary->get_key_len()), vlen(_vlen), ary(_ary), file_index(0)
    {
      key_len = bits_to_bytes(klen);
      val_len = bits_to_bytes(vlen);
      max_count     = (((uint64_t)1) << (8*val_len)) - 1;
      record_len    = key_len + val_len;
      nb_records    = ary->floor_block(_buffer_size / record_len, nb_blocks);

      buffers = new char[nb_records * record_len * threads];
      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        thread_info[i].token = i == 0;
        thread_info[i].buffer = &buffers[i * nb_records * record_len];
        thread_info[i].id = i;
        thread_info[i].self = this;
      }
      unique = distinct = total = 0;
    }

    ~compacted_dumper() {
      if(buffers) {
        delete[] buffers;
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
    bool write_header();
    bool update_stats();
  };

  template<typename storage_t, typename atomic_t>
  void compacted_dumper<storage_t,atomic_t>::dump() {
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

    for(uint_t i = 0; i < threads; i++)
      thread_info[i].token = i == 0;
    for(uint_t i = 0; i < threads; i++) {
      pthread_create(&thread_info[i].thread_id, NULL, dump_to_file_thread, 
                     &thread_info[i]);
    }

    for(uint_t i = 0; i < threads; i++)
      pthread_join(thread_info[i].thread_id, NULL);
    update_stats();
    out.close();
  }

  template<typename storage_t, typename atomic_t>
  void compacted_dumper<storage_t,atomic_t>::dump_to_file(struct thread_info_t *my_info) {
    size_t                i;
    uint64_t              count;
    struct thread_info_t *next_info = &thread_info[(my_info->id + 1) % threads];
    uint64_t              _unique   = 0;
    uint64_t              _distinct = 0;
    uint64_t              _total    = 0;
    atomic_t              atomic;

    if(my_info->token)
      write_header();

    for(i = my_info->id; i * nb_records < ary->get_size(); i += threads) {
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
      out.write((char *)my_info->buffer, ptr - my_info->buffer);
          
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

  template<typename storage_t, typename atomic_t>
  bool compacted_dumper<storage_t,atomic_t>::write_header() {
    jellyfish::compacted_hash::header head;

    memset(&head, '\0', sizeof(head));
    head.key_len = klen;
    head.val_len = val_len;
    head.size = ary->get_size();
    head.max_reprobes = ary->get_max_reprobe_offset();
    out.write((char *)&head, sizeof(head));
    ary->write_ary_header(out);
    return out.good();
  }

  template<typename storage_t, typename atomic_t>
  bool compacted_dumper<storage_t,atomic_t>::update_stats() {
    jellyfish::compacted_hash::header head;
    head.key_len = klen;
    head.val_len = val_len;
    head.size = ary->get_size();
    head.max_reprobes = ary->get_max_reprobe_offset();
    head.unique = unique;
    head.distinct = distinct;
    head.total = total;
    out.seekp(0);
    out.write((char *)&head, sizeof(head));
    return out.good();
  }
}
