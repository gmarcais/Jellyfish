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

#ifndef __JELLYFISH_ALIGNED_VALUE_HPP__
#define __JELLYFISH_ALIGNED_VALUE_HPP__

#include <jellyfish/invertible_hash_array.hpp>
#include <jellyfish/direct_indexing_array.hpp>

namespace jellyfish {
  namespace aligned_values {
    template<typename _key_t, typename _val_t, typename atomic, typename mem_block_t>
    class array : public storage_t {
    public:
      typedef _key_t key_t;
      typedef _val_t val_t;

    private:
      typedef typename ::jellyfish::invertible_hash::array<key_t, atomic, mem_block_t> key_ary_t;
      typedef typename ::jellyfish::direct_indexing::array<size_t, val_t, atomic, mem_block_t> val_ary_t;
      
      key_ary_t keys;
      val_ary_t vals;

    public:
      array(size_t _size, uint_t _key_len, uint_t _reprobe_limit,
            size_t *_reprobes) :
        keys(_size, _key_len, 0, _reprobe_limit, _reprobes),
        vals(keys.get_lsize())
      { }

      array(char *keys_map, char *vals_map,
            size_t _size, uint_t _key_len, uint_t _reprobe_limit,
            size_t *_reprobes, SquareBinaryMatrix &hash_matrix,
            SquareBinaryMatrix &hash_inv_matrix) :
        keys(keys_map, _size, _key_len, 0, _reprobe_limit, _reprobes,
             hash_matrix, hash_inv_matrix),
        vals(vals_map, keys.get_lsize())
      { }


      void set_matrix(SquareBinaryMatrix &m) {
        keys.set_matrix(m);
      }
      size_t get_size() const { return keys.get_size(); }
      uint_t get_key_len() const { return keys.get_key_len(); }
      uint_t get_val_len() const { return keys.get_val_len(); }
      uint_t get_max_reprobe() const { return keys.get_max_reprobe(); }
      size_t get_max_reprobe_offset() const {
        return keys.get_max_reprobe_offset();
      }
      uint_t get_block_len() const { return keys.get_block_len(); }
      uint_t get_block_word_len() const { 
        return keys.get_block_word_len() + keys.get_block_len() * sizeof(val_t);
      }

      size_t floor_block(size_t entries, size_t &blocks) const {
        return keys.floor_block(entries, blocks);
      }
      void zero_keys(const size_t start, const size_t length) {
        keys.zero_blocks(start, length);
      }
      void zero_values(const size_t start, const size_t length) {
        vals.zero(start, length);
      }
      void write_keys_blocks(std::ostream *out, size_t start, size_t length) const {
        keys.write_blocks(out, start, length);
      }
      void write_values(std::ostream *out, size_t start, size_t length) const {
        vals.write(out, start, length);
      }
      void write_matrices(std::ostream *out) {
        keys.write_ary_header(out);
      }

      template<typename add_t>
      bool add(key_t key, const add_t &val, val_t *oval = 0) {
        bool   is_new;
        size_t id;
        
        if(!keys.set(key, &is_new, &id))
          return false;

        vals.add(id, val, oval);
        return true;
      }

      bool get_val(key_t key, val_t &val, bool full = true) const {
        key_t  v_ignore;
        size_t key_id;

        if(!keys.get_val(key, key_id, v_ignore, false))
          return false;

        vals.get_val(key_id, val);
        return true;
      }

      class iterator {
        typename key_ary_t::iterator              key_it;
        const val_ary_t                    *const vals;

      public:
        iterator(typename key_ary_t::iterator _key_it, const val_ary_t *_vals) :
          key_it(_key_it), vals(_vals) {}

        uint64_t  get_hash() const { return key_it.get_hash(); }
        uint64_t  get_pos() const { return key_it.get_pos(); }
        uint64_t  get_start() const { return key_it.get_start(); }
        uint64_t  get_end() const { return key_it.get_end(); }
        key_t     get_key() const { return key_it.get_key(); }
        val_t     get_val() const { return (*vals)[get_id()]; }
        size_t    get_id() const { return key_it.get_id(); }
        char     *get_dna_str() { return key_it.get_dna_str(); }
        bool      next() { return key_it.next(); }
      };
      iterator iterator_all() const { 
        return iterator(keys.iterator_all(), &vals);
      }
      iterator iterator_slice(size_t slice_number, size_t number_of_slice) const {
        return iterator(keys.iterator_slice(slice_number, number_of_slice),
                        &vals);
      }
    };
  }
}

#endif
