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

#include <jellyfish/err.hpp>
#include <jellyfish/dbg.hpp>
#include <jellyfish/dumper.hpp>
#include <jellyfish/heap.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/token_ring.hpp>

namespace jellyfish {
  template<typename storage_t, typename atomic_t>
  class sorted_dumper : public dumper_t, public thread_exec {
    typedef typename storage_t::overlap_iterator iterator;
    typedef compacted_hash::writer<storage_t> writer_t;
    typedef heap_t<iterator> oheap_t;
    typedef token_ring<locks::pthread::cond> token_ring_t;

    struct thread_info_t {
      writer_t             writer;
      oheap_t              heap;
      token_ring_t::token *token;
    };

    uint_t                 threads;
    std::string            file_prefix;
    size_t                 buffer_size;
    uint_t                 klen, vlen;
    uint_t                 key_len, val_len;
    size_t                 record_len, nb_records, nb_blocks;
    storage_t             *ary;
    int                    file_index;
    token_ring_t           tr;
    uint64_t               lower_count, upper_count;
    struct thread_info_t  *thread_info;
    uint64_t volatile      unique, distinct, total, max_count;
    std::ofstream         *out;
    locks::pthread::mutex  dump_mutex;
    bool                   one_file;

  public:
    // klen: key field length in bits in hash (i.e before rounding up to bytes)
    // vlen: value field length in bits
    sorted_dumper(uint_t _threads, const char *_file_prefix, size_t _buffer_size, 
                  uint_t _vlen, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix), buffer_size(_buffer_size),
      klen(_ary->get_key_len()), vlen(_vlen), ary(_ary), file_index(0),
      tr(), lower_count(0), upper_count(std::numeric_limits<uint64_t>::max()),
      one_file(false)
    {
      key_len    = bits_to_bytes(klen);
      val_len    = bits_to_bytes(vlen);
      record_len = key_len + val_len;
      nb_records = ary->floor_block(_buffer_size / record_len, nb_blocks);
      while(nb_records < ary->get_max_reprobe_offset()) {
        nb_records = ary->floor_block(2 * nb_records, nb_blocks);
      }

      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        //        thread_info[i].token = i == 0;
        thread_info[i].writer.initialize(nb_records, klen, vlen, ary);
        thread_info[i].heap.initialize(ary->get_max_reprobe_offset());
        thread_info[i].token = tr.new_token();
      }
      unique = distinct = total = max_count = 0;
    }

    ~sorted_dumper() {
      if(thread_info) {
        delete[] thread_info;
      }
    }
    
    bool get_one_file() const { return one_file; }
    void set_one_file(bool nv) { one_file = nv; }

    void set_lower_count(uint64_t l) { lower_count = l; }
    void set_upper_count(uint64_t u) { upper_count = u; }

    virtual void start(int i) { dump_to_file(i); }
    void dump_to_file(int i);

    virtual void _dump();
    void update_stats() {
      thread_info[0].writer.update_stats_with(out, unique, distinct, total, 
                                              max_count);
    }
  };

  template<typename storage_t, typename atomic_t>
  void sorted_dumper<storage_t,atomic_t>::_dump() {
    std::ofstream _out;
    assert(dump_mutex.try_lock());
    if(one_file) {
      _out.open(file_prefix.c_str());
    } else {
      open_next_file(file_prefix.c_str(), &file_index, _out);
    }
    out = &_out;
    unique = distinct = total = max_count = 0;
    tr.reset();
    thread_info[0].writer.write_header(out);
    exec_join(threads);
    ary->zero_blocks(0, nb_blocks); // zero out last group of blocks
    update_stats();
    _out.close();
    dump_mutex.unlock();
  }

  template<typename storage_t, typename atomic_t>
  void sorted_dumper<storage_t,atomic_t>::dump_to_file(int id) {
    size_t                i;
    struct thread_info_t *my_info = &thread_info[id];
    atomic_t              atomic;

    my_info->writer.reset_counters();

    for(i = id; i * nb_records < ary->get_size(); i += threads) {
      // fill up buffer
      iterator it(ary, i * nb_records, (i + 1) * nb_records);
      my_info->heap.fill(it);

      while(it.next()) {
        typename oheap_t::const_item_t item = my_info->heap.head();
        if(item->val >= lower_count && item->val <= upper_count)
          my_info->writer.append(item->key, item->val);
        my_info->heap.pop();
        my_info->heap.push(it);
      }

      while(my_info->heap.is_not_empty()) {
        typename oheap_t::const_item_t item = my_info->heap.head();
        if(item->val >= lower_count && item->val <= upper_count)
          my_info->writer.append(item->key, item->val);
        my_info->heap.pop();
      }

      my_info->token->wait();
      my_info->writer.dump(out);
      my_info->token->pass();

      // zero out memory
      if(i > 0)
        ary->zero_blocks(i * nb_blocks, nb_blocks);
    }

    atomic.add_fetch(&unique, my_info->writer.get_unique());
    atomic.add_fetch(&distinct, my_info->writer.get_distinct());
    atomic.add_fetch(&total, my_info->writer.get_total());
    atomic.set_to_max(&max_count, my_info->writer.get_max_count());
  }
}
