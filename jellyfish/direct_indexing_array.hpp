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

#ifndef __JELLYFISH_DIRECT_INDEXING_ARRAY_HPP__
#define __JELLYFISH_DIRECT_INDEXING_ARRAY_HPP__

namespace jellyfish {
  namespace direct_indexing {
    template<typename key_t, typename val_t, typename atomic_t, typename mem_block_t>
    class array {
      uint_t       key_len;
      size_t       size;
      mem_block_t  mem_block;
      val_t       *data;
      atomic_t     atomic;
      

    public:
      array(uint_t _key_len) :
        key_len(_key_len), size(((size_t)1) << key_len),
        mem_block(size * sizeof(val_t)),
        data((val_t *)mem_block.get_ptr())
      { }

      size_t get_size() const { return size; }
      uint_t get_key_len() const { return key_len; }
      uint_t get_val_len() const { return sizeof(val_t); }
      size_t get_max_reprobe_offset() const { return 1; }

      void write_ary_header(std::ostream *out) const {
        SquareBinaryMatrix id(key_len);
        id.init_identity();
        id.dump(out);
        id.dump(out);
      }
      void write_raw(std::ostream *out) const {}

      bool add(key_t key, val_t val) {
        static const val_t cap = (val_t)-1;
        val_t oval = data[key];

        while(oval != cap) {
          val_t nval = (val > ~oval ? cap : oval + val);
          nval = atomic.cas(&data[key], oval, nval);
          if(nval == oval)
            return true;
          oval = nval;
        }
        return true;
      }

      class iterator {
        const array *ary;
        size_t       start_id;
        size_t       nid;
        size_t       end_id;

      public:
        key_t   key;
        val_t   val;
        size_t  id;
        
        iterator(const array *_ary, size_t start, size_t end) :
          ary(_ary), start_id(start), nid(start),
          end_id(end > ary->get_size() ? ary->get_size() : end)
        {}

        void get_string(char *out) const {
          fasta_parser::mer_binary_to_string(key, ary->get_key_len() / 2, out);
        }
        uint64_t get_hash() const { return key; }
        uint64_t get_pos() const { return key; }
        uint64_t get_start() const { return start_id; }
        uint64_t get_end() const { return end_id; }
        
        bool next() {
          while((id = nid) < end_id) {
            nid++;
            val = ary->data[id];
            if(val) {
              key = id;
              return true;
            }
          }
          return false;
        }
      };
      friend class iterator;
      iterator iterator_all() const { return iterator(this, 0, get_size()); }
      iterator iterator_slice(size_t slice_number, size_t number_of_slice) const {
        size_t slice_size = get_size() / number_of_slice;
        return iterator(this, slice_number * slice_size, (slice_number + 1) * slice_size);
      }

      void zero(size_t start, size_t length) {
        if(start + length > size)
          length = size - start;
        memset(data + start, '\0', length);
      }
    };
  }
}

#endif
