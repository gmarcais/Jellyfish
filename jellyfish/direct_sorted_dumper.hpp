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

#include <limits>
#include <jellyfish/dumper.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/token_ring.hpp>

namespace jellyfish {
  template<typename storage_t, typename atomic_t>
  class direct_sorted_dumper : public dumper_t, public thread_exec {
    typedef typename storage_t::iterator               iterator;
    typedef typename compacted_hash::writer<storage_t> writer_t;
    typedef token_ring<locks::pthread::cond>           token_ring_t;

    struct thread_info_t {
      writer_t             writer;
      token_ring_t::token *token;
    };

    uint_t                threads;
    const char           *file_prefix;
    size_t                buffer_size;
    uint_t                klen, vlen;
    uint_t                key_len, val_len;
    size_t                record_len, nb_records;
    storage_t            *ary;
    int                   file_index;
    token_ring_t          tr;
    uint64_t              lower_count, upper_count;
    struct thread_info_t *thread_info;
    uint64_t volatile     unique, distinct, total, max_count;
    std::ofstream        *out;
    bool                  one_file;

  public:
    direct_sorted_dumper(uint_t _threads, const char *_file_prefix, 
                         size_t _buffer_size, uint_t _vlen, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix), buffer_size(_buffer_size),
      klen(_ary->get_key_len()), vlen(_vlen), ary(_ary),
      tr() , lower_count(0), upper_count(std::numeric_limits<uint64_t>::max()),
      one_file(false)
    {
      key_len    = bits_to_bytes(klen);
      val_len    = bits_to_bytes(vlen);
      record_len = key_len + val_len;
      nb_records = _buffer_size / record_len;
      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        thread_info[i].writer.initialize(nb_records, klen, vlen, ary);
        thread_info[i].token = tr.new_token();
      }
      unique = distinct = total = max_count = 0;
    }

    ~direct_sorted_dumper() {
      if(thread_info)
        delete[] thread_info;
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
  void direct_sorted_dumper<storage_t,atomic_t>::_dump() {
    std::ofstream _out;
    if(one_file) {
      _out.open(file_prefix);
    } else {
      open_next_file(file_prefix, &file_index, _out);
    }
    out = &_out;
    unique = distinct = total = max_count = 0;
    tr.reset();
    thread_info[0].writer.write_header(out);
    exec_join(threads);
    update_stats();
    _out.close();
  }

  template<typename storage_t, typename atomic_t>
  void direct_sorted_dumper<storage_t,atomic_t>::dump_to_file(int id) {
    size_t                i;
    struct thread_info_t *my_info = &thread_info[id];
    atomic_t              atomic;
      
    my_info->writer.reset_counters();

    for(i = id; i * nb_records < ary->get_size(); i += threads) {
      iterator it(ary, i * nb_records, (i + 1) * nb_records);
      while(it.next())
        if(it.get_val().bits() >= lower_count && it.get_val().bits() <= upper_count)
          my_info->writer.append(it.get_key(), it.get_val().bits());

      my_info->token->wait();
      my_info->writer.dump(out);
      my_info->token->pass();
      ary->zero(i * nb_records, nb_records);
    }
    atomic.add_fetch(&unique, my_info->writer.get_unique());
    atomic.add_fetch(&distinct, my_info->writer.get_distinct());
    atomic.add_fetch(&total, my_info->writer.get_total());
    atomic.set_to_max(&max_count, my_info->writer.get_max_count());
  }
}
