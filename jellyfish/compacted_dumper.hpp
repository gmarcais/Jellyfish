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

#include <jellyfish/dumper.hpp>

namespace jellyfish {
  template<typename storage_t, typename atomic_t>
  class compacted_dumper : public dumper_t {
    define_error_class(ErrorWriting);
    typedef typename storage_t::iterator iterator;
    typedef compacted_hash::writer<storage_t> writer_t;
    struct thread_info_t {
      pthread_t             thread_id;
      uint_t                id;
      locks::pthread::cond  cond;
      volatile bool         token;
      writer_t              writer;
      compacted_dumper     *self;
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

  public:
    // klen: key field length in bits in hash (i.e before rounding up to bytes)
    // vlen: value field length in bits
    compacted_dumper(uint_t _threads, const char *_file_prefix, size_t _buffer_size, 
                     uint_t _vlen, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix), buffer_size(_buffer_size),
      klen(_ary->get_key_len()), vlen(_vlen), ary(_ary), file_index(0)
    {
      std::cerr << "Compacted dumper init" << std::endl;
      key_len    = bits_to_bytes(klen);
      val_len    = bits_to_bytes(vlen);
      max_count  = (((uint64_t)1) << (8*val_len)) - 1;
      record_len = key_len + val_len;
      nb_records = ary->floor_block(_buffer_size / record_len, nb_blocks);

      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        thread_info[i].token = i == 0;
        thread_info[i].writer.initialize(nb_records, ary->get_key_len(), vlen, ary);
        thread_info[i].id = i;
        thread_info[i].self = this;
      }
      unique = distinct = total = 0;
    }

    ~compacted_dumper() {
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
      thread_info[0].writer.update_stats_with(&out, unique, distinct, total);
    }
  };

  template<typename storage_t, typename atomic_t>
  void compacted_dumper<storage_t,atomic_t>::dump() {
    static const long file_len = pathconf("/", _PC_PATH_MAX);
    std::cerr << "dump()" << std::endl;
    char file[file_len + 1];
    file[file_len] = '\0';
    int off = snprintf(file, file_len, "%s", file_prefix.c_str());
    if(off < 0)
      eraise(ErrorWriting) << "Error creating output path" << err::no;
    if(off > 0 && off < file_len) {
      int _off = snprintf(file + off, file_len - off, "_%uld", file_index++);
      if(_off < 0)
        eraise(ErrorWriting) << "Error creating output path" << err::no;
      off += _off;
    }
    if(off >= file_len)
      eraise(ErrorWriting) << "File path is too long";
      
    
    //    out.exceptions(std::ios::eofbit|std::ios::failbit|std::ios::badbit);
    std::cerr << "Open " << file << std::endl;
    out.open(file);
    if(!out.good())
      eraise(ErrorWriting) << "'" << file << "': " 
                           << "Can't open file for writing" << err::no;


    out.write("JFLISTDN", 8);
    unique = distinct = total = 0;
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
    struct thread_info_t *next_info = &thread_info[(my_info->id + 1) % threads];
    atomic_t              atomic;

    if(my_info->token)
      my_info->writer.write_header(&out);

    for(i = my_info->id; i * nb_records < ary->get_size(); i += threads) {
      // fill up buffer
      iterator it(ary, i * nb_records, (i + 1) * nb_records);

      while(it.next()) {
        my_info->writer.append(it.key, it.val);
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
      ary->zero_blocks(i * nb_blocks, nb_blocks);
    }
    atomic.add_fetch(&unique, my_info->writer.get_unique());
    atomic.add_fetch(&distinct, my_info->writer.get_distinct());
    atomic.add_fetch(&total, my_info->writer.get_total());
  }
}
