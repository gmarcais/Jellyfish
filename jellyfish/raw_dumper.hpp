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

#ifndef __RAW_DUMPER_HPP__
#define __RAW_DUMPER_HPP__

#include <jellyfish/dumper.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/token_ring.hpp>
#include <jellyfish/locks_pthread.hpp>

namespace jellyfish {
  template<typename storage_t>
  class raw_hash_dumper : public dumper_t, public thread_exec {
    typedef token_ring<locks::pthread::cond> token_ring_t;

    struct header {
      char     type[8];
      uint64_t key_len;
      uint64_t val_len;
      uint64_t size;
      uint64_t max_reprobe;
    };
    struct thread_info_t {
      token_ring_t::token *token;
    };
    const uint_t         threads;
    const std::string    file_prefix;
    storage_t     *const ary;
    int                  file_index;
    token_ring_t         tr;
    
    struct thread_info_t *thread_info;
    size_t                nb_records, nb_blocks;
    std::ofstream        *out;

  public:
    raw_hash_dumper(uint_t _threads, const char *_file_prefix, size_t chunk_size, storage_t *_ary) :
      threads(_threads), file_prefix(_file_prefix),
      ary(_ary), file_index(0), tr()
    {
      nb_records = ary->floor_block(chunk_size / ary->get_block_len(), nb_blocks);
      while(nb_records < ary->get_max_reprobe_offset()) {
        nb_records = ary->floor_block(2 * nb_records, nb_blocks);
      }
      thread_info = new struct thread_info_t[threads];
      for(uint_t i = 0; i < threads; i++) {
        thread_info[i].token = tr.new_token();
      }
    }
    
    ~raw_hash_dumper() {
      if(thread_info) {
        delete[] thread_info;
      }
    }
    
    virtual void start(int i) { dump_to_file(i); }
    void dump_to_file(int i);
    void write_header();

    virtual void _dump();
  };

  template<typename storage_t>
  void raw_hash_dumper<storage_t>::_dump() {
    std::ofstream _out;
    open_next_file(file_prefix.c_str(), file_index, _out);
    out = &_out;
    tr.reset();
    write_header();
    exec_join(threads);
    _out.close();
  }

  template<typename storage_t>
  void raw_hash_dumper<storage_t>::dump_to_file(int id) {
    size_t i;
    struct thread_info_t *my_info = &thread_info[id];
    
    for(i = id; i * nb_records < ary->get_size(); i += threads) {
      my_info->token->wait();
      ary->write_blocks(out, i * nb_blocks, nb_blocks);
      my_info->token->pass();
      ary->zero_blocks(i * nb_blocks, nb_blocks);
    }
  }

  template<typename storage_t>
  void raw_hash_dumper<storage_t>::write_header() {
    struct header header;
    memcpy(&header.type, "JFRHSHDN", sizeof(header.type));
    header.key_len = ary->get_key_len();
    header.val_len = ary->get_val_len();
    header.size = ary->get_size();
    header.max_reprobe = ary->get_max_reprobe();
    out->write((char *)&header, sizeof(header));
    ary->write_ary_header(out);
  }
}

#endif
