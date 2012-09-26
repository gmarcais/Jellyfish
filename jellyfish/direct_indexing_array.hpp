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
    public:
      typedef typename val_t::bits_t bits_t;

      uint_t       key_len;
      size_t       size;
      mem_block_t  mem_block;
      bits_t      *data;
      atomic_t     atomic;
      

    public:
      explicit array(uint_t _key_len) :
        key_len(_key_len), size(((size_t)1) << key_len),
        mem_block(size * sizeof(bits_t)),
        data((bits_t *)mem_block.get_ptr())
      { }

      array(char *map, uint_t _key_len) :
        key_len(_key_len), size(((size_t)1) << key_len),
        data((bits_t *)map)
      { }

      size_t get_size() const { return size; }
      uint_t get_key_len() const { return key_len; }
      uint_t get_val_len() const { return sizeof(bits_t); }
      size_t get_max_reprobe_offset() const { return 1; }

      void write_ary_header(std::ostream *out) const {
        SquareBinaryMatrix id(key_len);
        id.init_identity();
        id.dump(out);
        id.dump(out);
      }
      void write_raw(std::ostream *out) const {}

      template<typename add_t>
      bool add(key_t key, const add_t &val, val_t *_oval = 0) {
        bits_t oval = data[key];
        val_t nval = val_t(oval) + val;

        while(true) {
          bits_t noval = atomic.cas(&data[key], oval, nval.bits());
          if(noval == oval) {
            if(_oval)
              *_oval = val_t(oval);
            return true;
          }
          oval = noval;
          nval = val_t(oval) + val;
        }
        return true;
      }

      bool get_val(key_t key, val_t &val, bool full = true) const {
        val = data[key];
        return true;
      }

      class iterator {
        const array *ary;
        size_t       start_id;
        size_t       nid;
        size_t       end_id;
        key_t        key;
        bits_t       val;
        size_t       id;

      public:
        iterator(const array *_ary, size_t start, size_t end) :
          ary(_ary), start_id(start), nid(start),
          end_id(end > ary->get_size() ? ary->get_size() : end)
        {}

        void get_string(char *out) const {
          parse_dna::mer_binary_to_string(key, ary->get_key_len() / 2, out);
        }
        uint64_t get_hash() const { return key; }
        uint64_t get_pos() const { return key; }
        uint64_t get_start() const { return start_id; }
        uint64_t get_end() const { return end_id; }
        key_t    get_key() const { return key; }
        val_t    get_val() const { return val_t(val); }
        size_t   get_id() const { return id; }
        
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
        std::pair<size_t, size_t> res = slice(slice_number, number_of_slice, get_size());
        return iterator(this, res.first, res.second);
      }

      /**
       * Zero out entries in [start, start+length).
       */
      void zero(size_t start, size_t length) {
        if(start >= size)
          return;
        if(start + length > size)
          length = size - start;
        memset(data + start, '\0', length * sizeof(*data));
      }

      void write(std::ostream *out, const size_t start, size_t length) const {
        if(start >= size)
          return;
        if(start + length > size)
          length = size - start;
        out->write((char *)(data + start), length * sizeof(*data));
      }

      val_t operator[](key_t key) const { return val_t(data[key]); }
    };
  }
}

#endif
