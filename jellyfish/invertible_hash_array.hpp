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

#ifndef __JELLYFISH_REVERSIBLE_HASH__
#define __JELLYFISH_REVERSIBLE_HASH__

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <iostream>
#include <utility>
#include <exception>
#include <assert.h>
#include <jellyfish/storage.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/storage.hpp>
#include <jellyfish/offsets_key_value.hpp>
#include <jellyfish/parse_dna.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/simple_circular_buffer.hpp>

namespace jellyfish {
  namespace invertible_hash {
    define_error_class(InvalidMap);
    define_error_class(ErrorAllocation);
    define_error_class(InvalidMatrix);

    /* Contains an integer, the reprobe limit. It is capped based on
     * the reprobe strategy to not be bigger than the size of the hash
     * array.
     */
    class reprobe_limit_t {
      uint_t limit;
    public:
      reprobe_limit_t(uint_t _limit, size_t *_reprobes, size_t _size) :
        limit(_limit)
      {
        while(_reprobes[limit] >= _size && limit >= 1)
          limit--;
      }
      inline uint_t val() const { return limit; }
    };

    /* (key,value) pair bit-packed array.  It implements the logic of the
     * packed hash except for size doubling. Setting or incrementing a key
     * will return false if the hash is full. No memory management is done
     * in this class either.
     *
     * The hash function is assumed to be invertible. The key is not
     * directly stored in the hash. Let h = hash(key), size_table =
     * 2**k and key_len = h+k. In the key field is written
     *
     * h high bits of h, reprobe value + 1
     *
     * The +1 for the reprobe value is so that the field is guaranteed
     * not to be zero.
     */
    template <typename word, typename atomic_t, typename mem_block_t>
    class array : public storage_t {
      typedef typename Offsets<word>::offset_t offset_t;
      uint_t             lsize;    // log of size
      size_t             size, size_mask;
      reprobe_limit_t    reprobe_limit;
      uint_t             key_len;  // original key len
      word               key_mask; // mask for high bits of hash(key)
      uint_t             key_off;  // offset in key field for reprobe value
      Offsets<word>      offsets;  // key len reduced by size of hash array
      mem_block_t        mem_block;
      word              *data;
      atomic_t           atomic;
      size_t            *reprobes;
      SquareBinaryMatrix hash_matrix;
      SquareBinaryMatrix hash_inverse_matrix;
      struct header {
        uint64_t klen;
        uint64_t clen;
        uint64_t size;
        uint64_t reprobe_limit;
      };

    public:
      typedef word key_t;
      typedef word val_t;

      array(size_t _size, uint_t _key_len, uint_t _val_len,
            uint_t _reprobe_limit, size_t *_reprobes) :
        lsize(ceilLog2(_size)), size(((size_t)1) << lsize),
        size_mask(size - 1),
        reprobe_limit(_reprobe_limit, _reprobes, size), key_len(_key_len),
        key_mask(key_len <= lsize ? 0 : (((word)1) << (key_len - lsize)) - 1),
        key_off(key_len <= lsize ? 0 : key_len - lsize),
        offsets(key_off + bitsize(reprobe_limit.val() + 1), _val_len,
                reprobe_limit.val() + 1),
        mem_block(div_ceil(size, (size_t)offsets.get_block_len()) * offsets.get_block_word_len() * sizeof(word)),
        data((word *)mem_block.get_ptr()), reprobes(_reprobes),
        hash_matrix(key_len), 
        hash_inverse_matrix(hash_matrix.init_random_inverse())
      {
        if(!data)
          eraise(ErrorAllocation) << "Failed to allocate " 
                                 << (div_ceil(size, (size_t)offsets.get_block_len()) * offsets.get_block_word_len() * sizeof(word))
                                 << " bytes of memory";
      }
      
      // TODO: This parsing should be done in another class and use
      // the following constructor.
      // array(char *map, size_t length) :
      //   hash_matrix(0), hash_inverse_matrix(0) {
      //   if(length < sizeof(struct header))
      //     eraise(InvalidMap) << "File truncated";
      //   struct header *header = (struct header *)map;
      //   size = header->size;
      //   if(size != (1UL << floorLog2(size)))
      //     eraise(InvalidMap) << "Size '" << size << "' is not a power of 2";
      //   lsize = ceilLog2(size);
      //   size_mask = size - 1;
      //   reprobe_limit = header->reprobe_limit;
      //   key_len = header->klen;
      //   if(key_len > 64 || key_len == 0)
      //     eraise(InvalidMap) << "Invalid key length '" << key_len << "'";
      //   offsets.init(key_len + bitsize(reprobe_limit + 1) - lsize, header->clen,
      //                reprobe_limit);
      //   key_mask = (((word)1) << (key_len - lsize)) - 1;
      //   key_off = key_len - lsize;
      //   map += sizeof(struct header);
      //   // reprobes = new size_t[header->reprobe_limit + 1];
      //   // TODO: should that be in the database file?
      //   reprobes = jellyfish::quadratic_reprobes;
      //   // memcpy(reprobes, map, sizeof(size_t) * (header->reprobe_limit + 1));
      //   // map += sizeof(size_t) * (header->reprobe_limit + 1);
      //   map += hash_matrix.read(map);
      //   if((uint_t)hash_matrix.get_size() != key_len)
      //     eraise(InvalidMatrix) << "Size of hash matrix '" << hash_matrix.get_size() 
      //                          << "' not equal to key length '" << key_len << "'";
      //   map += hash_inverse_matrix.read(map);
      //   if((uint_t)hash_inverse_matrix.get_size() != key_len)
      //     eraise(InvalidMatrix) << "Size of inverse hash matrix '" << hash_inverse_matrix.get_size()
      //                          << "' not equal to key length '" << key_len << "'";
      //   if((size_t)map & 0x7)
      //     map += 0x8 - ((size_t)map & 0x7); // Make sure aligned for 64bits word. TODO: use alignof?
      //   data = (word *)map;
      // }

      // Assume _size is already a power of 2
      // map must point to a memory area written by "write_blocks". No header
      array(char *map, size_t _size, uint_t _key_len, uint_t _val_len,
            uint_t _reprobe_limit, size_t *_reprobes,
            SquareBinaryMatrix &_hash_matrix, 
            SquareBinaryMatrix &_hash_inverse_matrix) :
        lsize(ceilLog2(_size)), size(_size), size_mask(size-1),
        reprobe_limit(_reprobe_limit, _reprobes, size), key_len(_key_len),
        key_mask(key_len <= lsize ? 0 : (((word)1) << (key_len - lsize)) - 1),
        key_off(key_len <= lsize ? 0 : key_len - lsize),
        offsets(key_off + bitsize(reprobe_limit.val() + 1), _val_len,
                reprobe_limit.val() + 1),
        data((word *)map), reprobes(_reprobes),
        hash_matrix(_hash_matrix), hash_inverse_matrix(_hash_inverse_matrix)
      { }

      ~array() { }

      // Lock in memory
      int lock() {
        return mem_block.lock();
      }

      void set_matrix(SquareBinaryMatrix &m) {
        if((uint_t)m.get_size() != key_len)
          eraise(InvalidMatrix) << "Size of matrix '" << m.get_size() 
                               << "' not equal to key length '" << key_len << "'";
        hash_matrix = m;
        hash_inverse_matrix = m.inverse();
      }

      size_t get_size() const { return size; }
      size_t get_lsize() const { return lsize; }
      uint_t get_key_len() const { return key_len; }
      uint_t get_val_len() const { return offsets.get_val_len(); }
      
      uint_t get_max_reprobe() const { return reprobe_limit.val(); }
      size_t get_max_reprobe_offset() const { 
        return reprobes[reprobe_limit.val()]; 
      }

      
      uint_t get_block_len() const { return offsets.get_block_len(); }
      uint_t get_block_word_len() const { return offsets.get_block_word_len(); }
      size_t floor_block(size_t entries, size_t &blocks) const {
	return offsets.floor_block(entries, blocks);
      }

    private:
      void block_to_ptr(const size_t start, const size_t blen,
                        char **start_ptr, size_t *memlen) const {
	*start_ptr = (char *)(data + start * offsets.get_block_word_len());
	char *end_ptr = (char *)mem_block.get_ptr() + mem_block.get_size();

	if(*start_ptr >= end_ptr) {
          *memlen = 0;
	  return;
        }
	*memlen = blen * offsets.get_block_word_len() * sizeof(word);
	if(*start_ptr + *memlen > end_ptr)
	  *memlen = end_ptr - *start_ptr;
      }

    public:
      /**
       * Zero out blocks in [start, start+length), where start and
       * length are given in number of blocks.
       **/
      void zero_blocks(const size_t start, const size_t length) {
        char   *start_ptr;
        size_t  memlen;
        block_to_ptr(start, length, &start_ptr, &memlen);
	memset(start_ptr, '\0', memlen);
      }

      /**
       * Write to out blocks [start, start+length).
       */
      void write_blocks(std::ostream *out, const size_t start, const size_t length) const {
        char   *start_ptr;
        size_t  memlen;
        block_to_ptr(start, length, &start_ptr, &memlen);
	out->write(start_ptr, memlen);
      }

      // Iterator
      class iterator {
      protected:
        const array	*ary;
        size_t		 start_id, nid, end_id;
        uint64_t	 mask;
	char		 dna_str[33];
      public:
        word     key;
        word     val;
        size_t   id;
        uint64_t hash;

        iterator(const array *_ary, size_t start, size_t end) :
          ary(_ary),
          start_id(start > ary->get_size() ? ary->get_size() : start),
          nid(start), 
	  end_id(end > ary->get_size() ? ary->get_size() : end),
          mask(ary->get_size() - 1)
        {}
        
        void get_string(char *out) const {
          parse_dna::mer_binary_to_string(key, ary->get_key_len() / 2, out);
        }
        uint64_t get_hash() const { return hash; }
        uint64_t get_pos() const { return hash & mask; }
	uint64_t get_start() const { return start_id; }
	uint64_t get_end() const { return end_id; }
	word get_key() const { return key; }
	word get_val() const { return val; }
	size_t get_id() const { return id; }
	char *get_dna_str() {
	  parse_dna::mer_binary_to_string(key, ary->get_key_len() / 2, dna_str);
	  return dna_str;
	}

        bool next() {
          bool success;
          while((id = nid) < end_id) {
            nid++;
            success = ary->get_key_val_full(id, key, val);
            if(success) {
              hash = (key & ary->key_mask) << ary->lsize;
              uint_t reprobep = (key >> ary->key_off) - 1;
              hash |= (id - (reprobep > 0 ? ary->reprobes[reprobep] : 0)) & ary->size_mask;
              key = ary->hash_inverse_matrix.times(hash);
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

      /* Why on earth doesn't inheritance with : public iterator work
         here? Resort to copying code. Arrrgggg....
       */
      class overlap_iterator {
      protected:
        const array *ary;
        uint64_t     mask;
        size_t       start_id, end_id, start_oid;
        size_t       moid, oid;
      public:
        word     key;
        word     val;
        size_t   id;
        uint64_t hash;

        overlap_iterator(const array *_ary, size_t start, size_t end) :
          ary(_ary),
          mask(ary->get_size() - 1),
          start_id(start),
          end_id(end > ary->get_size() ? ary->get_size() : end),
          start_oid(start),
          moid(end_id - start_id + ary->get_max_reprobe_offset()),
          oid(0)
        {
          // Adjust for very small arrays and it overlaps with itself
          if(moid > ary->get_size() - start_id) {
            size_t last_id = (start_id + moid) % mask;
            if(last_id > start_id)
              moid -= last_id - start_id - 1;
          }
        }
        
        void get_string(char *out) const {
          parse_dna::mer_binary_to_string(key, ary->get_key_len() / 2, out);
        }
        uint64_t get_hash() const { return hash; }
        uint64_t get_pos() const { return hash & mask; }
	uint64_t get_start() const { return start_id; }
	uint64_t get_end() const { return end_id; }

        bool next() {
          bool success;
          while(oid < moid) {
            id = (start_oid + oid++) & mask;
            success = ary->get_key_val_full(id, key, val);
            if(success) {
              hash = (key & ary->key_mask) << ary->lsize;
              uint_t reprobep = (key >> ary->key_off) - 1;
              hash |= (id - (reprobep > 0 ? ary->reprobes[reprobep] : 0)) & ary->size_mask;
              if(get_pos() < start_id || get_pos() >= end_id)
                continue;
              key = ary->hash_inverse_matrix.times(hash);
              return true;
            }
          }
          return false;
        }
      };
      friend class overlap_iterator;

      /* Return whether the entry is empty and if not, it returns the
       * key and if it has the large bit set.
       */
      void get_entry_stats(size_t id, bool &empty, word &key, bool &large) const {
        word           *w, *kvw = NULL;
        const offset_t *o, *lo  = NULL;

        w = offsets.get_word_offset(id, &o, &lo, data);
        kvw = w + o->key.woff;
        key = *kvw;
        large = key & o->key.lb_mask;
        if(large)
          o = lo;
        if(o->key.mask2) {
          key = (key & o->key.mask1 & ~o->key.sb_mask1) >> o->key.boff;
          key |= ((*(kvw+1)) & o->key.mask2 & ~o->key.sb_mask2) << o->key.shift;
        } else {
          key = (key & o->key.mask1) >> o->key.boff;
        }
        empty = key == 0;
      }

      /*
       * Return the key and value at position id. If the slot at id is
       * empty, returns false. If the slot at position id has the large
       * bit set, the key is resolved by looking backward in the
       * table. The value returned on the other hand is the value at
       * position id. No summation of the other entries for the key is
       * done.
       */
      bool get_key_val(size_t id, word &key, word &val) const {
        word           *w, *kvw, *fw = NULL;
        const offset_t *o, *lo, *fo  = NULL;
        bool            large;
        uint_t          overflows;

        overflows = 0;
        while(true) {
          w = offsets.get_word_offset(id, &o, &lo, data);
          kvw = w + o->key.woff;
          key = *kvw;
          large = key & o->key.lb_mask;
          if(large)
            o = lo;
          if(o->key.mask2) {
            key = (key & o->key.mask1 & ~o->key.sb_mask1) >> o->key.boff;
            key |= ((*(kvw+1)) & o->key.mask2 & ~o->key.sb_mask2) << o->key.shift;
          } else {
            key = (key & o->key.mask1) >> o->key.boff;
          }

          // Save offset and word for value retrieval
          if(!fo) {
            fo = o;
            fw = w;
          }

          if(large) {
            if(key)
              id -= reprobes[key];
            id = (id - reprobes[0]) & size_mask;
            overflows++;
          } else {
            break;
          }
        }
        if(!key)
          return false;

        kvw = fw + fo->val.woff;
        val = ((*kvw) & fo->val.mask1) >> fo->val.boff;
        if(fo->val.mask2) {
          val |= ((*(kvw+1)) & fo->val.mask2) << fo->val.shift;
        }
        if(overflows > 0) {
          val <<= offsets.get_val_len();
          if(--overflows > 0)
            val <<= offsets.get_lval_len() * overflows;
        }
        return true;
      }

      /*
       * Return the key and value at position id. If the slot at id is
       * empty or has the large bit set, returns false. Otherwise,
       * returns the key and the value is the sum of all the entries
       * in the hash table for that key. I.e., the table is search
       * forward for entries with large bit set pointing back to the
       * key at id, and all those values are summed up.
       */
      bool get_key_val_full(size_t id, word &key, word &val,
                            bool carry_bit = false) const {
        const offset_t	*o, *lo;
        word		*w, *kvw, nkey, nval;
        uint_t		 reprobe = 0, overflows = 0;
        size_t		 cid;

        w   = offsets.get_word_offset(id, &o, &lo, data);
        kvw = w + o->key.woff;
        key = *kvw;
        if(key & o->key.lb_mask)
          return false;
        if(o->key.mask2) {
          if((key & o->key.sb_mask1) == 0)
            return false;
          key  = (key & o->key.mask1 & ~o->key.sb_mask1) >> o->key.boff;
          key |= ((*(kvw+1)) & o->key.mask2 & ~o->key.sb_mask2) << o->key.shift; 
        } else {
          key = (key & o->key.mask1) >> o->key.boff;
          if(key == 0)
            return false;
        }

        kvw = w + o->val.woff;
        val = ((*kvw) & o->val.mask1) >> o->val.boff;
        if(o->val.mask2)
          val |= ((*(kvw+1)) & o->val.mask2) << o->val.shift;

        if(carry_bit) {
          bool do_reprobe = val & 0x1;
          val >>= 1;
          if(!do_reprobe)
            return true;
        }

        // Resolve value
        reprobe = 0;
        cid = id = (id + reprobes[0]) & size_mask;
        while(reprobe <= reprobe_limit.val()) {
          if(reprobe)
            cid  = (id + reprobes[reprobe]) & size_mask;

          w    = offsets.get_word_offset(cid, &o, &lo, data);
          kvw  = w + o->key.woff;
          nkey = *kvw;
          if(nkey & o->key.lb_mask) {
            if(lo->key.mask2) {
              nkey  = (nkey & lo->key.mask1 & ~lo->key.sb_mask1) >> lo->key.boff;
              nkey |= ((*(kvw+1)) & lo->key.mask2 & ~lo->key.sb_mask2) << lo->key.shift;
            } else {
              nkey = (nkey & lo->key.mask1) >> lo->key.boff;
            }
            if(nkey == reprobe) {
              kvw = w + lo->val.woff;
              nval = ((*kvw) & lo->val.mask1) >> lo->val.boff;
              if(lo->val.mask2)
                nval |= ((*(kvw+1)) & lo->val.mask2) << lo->val.shift;
              bool do_reprobe = true;
              if(carry_bit) {
                do_reprobe = nval & 0x1;
                nval >>= 1;
              }

              nval <<= offsets.get_val_len();
              nval <<= offsets.get_lval_len() * overflows;
              val += nval;

              if(!do_reprobe)
                return true;

              overflows++;
              reprobe = 0;
              cid = id = (cid + reprobes[0]) & size_mask;
              continue;
            }
          } else {
            if(o->key.mask2) {
              if((nkey & o->key.sb_mask1) == 0)
                return true;
            } else {
              if((nkey & o->key.mask1) == 0)
                return true;
            }
          }

          reprobe++;
        }

        return true;
      }

      inline bool get_val(const word key, word &val, bool full = false,
                          bool carry_bit = false) const {
        uint64_t hash = hash_matrix.times(key);
        size_t key_id;
        return _get_val(hash & size_mask, key_id, (hash >> lsize) & key_mask, val, 
                        full, carry_bit);
      }

      inline bool get_val(const word key, size_t &key_id, word &val, 
                          bool full = false, bool carry_bit = false) const {
        uint64_t hash = hash_matrix.times(key);
        return _get_val(hash & size_mask, key_id, (hash >> lsize) & key_mask, val,
                        full, carry_bit);
      }

      // /* InputIterator points to keys (words). OutputIterator points
      //    to struct containing at least the fields { bool found; size_t
      //    key_id; word val; }.
      //  */
      // template<typename InputIterator, typename OutputIterator>
      // void get_multi_val(InputIterator key, const InputIterator& key_end,
      //                    OutputIterator val, bool full, bool carry_bit) const {
      //   uint64_t        phash, chash;
      //   const offset_t *po, *plo, *co, *clo;
      //   const word     *pw, *cw;

      //   if(key == key_end)
      //     return;

      //   // Call __get_val with a delay. Let prefetch work while we
      //   // compute the hash/get the previous key.
      //   phash = hash_matrix.times(*key);
      //   pw    = offsets.get_word_offset(phash & size_mask, &po, &plo, data);
      //   //__builtin_prefetch(pw + po->key.woff, 0, 3);

      //   for(++key; key != key_end; ++key, ++val) {
      //     chash = hash_matrix.times(*key);
      //     cw    = offsets.get_word_offset(chash & size_mask, &co, &clo, data);
      //     //__builtin_prefetch(cw + co->key.woff, 0, 3);

      //     val->found = __get_val(phash & size_mask, val->key_id, 
      //                            (phash >> lsize) & key_mask, val->val,
      //                            full, carry_bit, pw, po, plo);


      //     pw    = cw;
      //     po    = co;
      //     plo   = clo;
      //     phash = chash;
      //   }
      //   // Last one
      //   val->found =  __get_val(phash & size_mask, val->key_id, 
      //                           (phash >> lsize) & key_mask, val->val,
      //                           full, carry_bit, pw, po, plo);
      // }

      struct prefetch_info {
        const word*     w;
        const offset_t *o, *lo;
      };
      typedef simple_circular_buffer::pre_alloc<prefetch_info, 8> prefetch_buffer;

      void warm_up_cache(prefetch_buffer& buffer, size_t id, bool load_lo) const {
        buffer.clear();
        for(int i = 0; i < buffer.capacity(); ++i) {
          buffer.push_back();
          prefetch_info& info = buffer.back();
          size_t         cid  = (id + (i > 0 ? reprobes[i] : 0)) & size_mask;
          info.w              = offsets.get_word_offset(cid, &info.o, &info.lo, data);
          __builtin_prefetch(info.w + info.o->key.woff, 0, 1);
          __builtin_prefetch(info.o, 0, 3);
          if(load_lo)
            __builtin_prefetch(info.lo, 0, 3);
        }
      }

      void prefetch_next(prefetch_buffer& buffer, size_t id, uint_t reprobe, bool load_lo) const {
        buffer.pop_front();
        if(reprobe + buffer.capacity() <= reprobe_limit.val()) {
          buffer.push_back();
          prefetch_info& info = buffer.back();
          size_t         fid  = (id + reprobes[reprobe + buffer.capacity() - 1]) & size_mask;
          info.w = offsets.get_word_offset(fid, &info.o, &info.lo, data);
          __builtin_prefetch(info.w + info.o->key.woff, 0, 1);
          __builtin_prefetch(info.o, 0, 3);
          if(load_lo)
            __builtin_prefetch(info.lo, 0, 3);
        }

      }

      bool _get_val(const size_t id, size_t &key_id, const word key, word &val, 
                           bool full = false, bool carry_bit = false) const {
        // Buffer for pre-cached information
        prefetch_info info_ary[prefetch_buffer::capacity()];
        prefetch_buffer buffer(info_ary);
        warm_up_cache(buffer, id, false);

        return __get_val(id, key_id, key, val, full, carry_bit, buffer);
      }

      bool __get_val(const size_t id, size_t &key_id, const word key, word &val, 
                     const bool full, bool carry_bit,
                     prefetch_buffer& buffer) const {
        const word     *kvw;
        word            nkey, nval;
        size_t          cid     = id;
        uint_t          reprobe = 0;
        word            akey    = key | ((word)1 << key_off);

        // Find key
        const offset_t *o, *lo;
        const word* w;
        while(true) {
          prefetch_info& info = buffer.front();
          w                   = info.w;
          o                   = info.o;
          kvw                 = w + o->key.woff;
          nkey                = *kvw;
      
          if(!(nkey & o->key.lb_mask)) {
            if(o->key.mask2) {
              nkey = (nkey & o->key.mask1 & ~o->key.sb_mask1) >> o->key.boff;
              nkey |=  ((*(kvw+1)) & o->key.mask2 & ~o->key.sb_mask2) << o->key.shift;
            } else {
              nkey = (nkey & o->key.mask1) >> o->key.boff;
            }
            if(nkey == akey)
              break;
          }
          if(++reprobe > reprobe_limit.val())
            return false;
          // Do reprobe
          cid  = (id + reprobes[reprobe]) & size_mask;
          akey = key | ((reprobe + 1) << key_off);

          prefetch_next(buffer, id, reprobe, false);
        }

        // Get value
        kvw = w + o->val.woff;
        val = ((*kvw) & o->val.mask1) >> o->val.boff;
        if(o->val.mask2) {
          val |= ((*(kvw+1)) & o->val.mask2) << o->val.shift;
        }
        bool do_reprobe = true;
        if(carry_bit) {
          do_reprobe   = val & 0x1;
          val        >>= 1;
        }
        key_id = cid;

        // Eventually get large values...  TODO: this seems buggy. It
        // only looks for large values once, not as many times as
        // needed.
        if(full && do_reprobe) {
          const size_t bid = (cid + reprobes[0]) & size_mask;
          cid = bid;

          warm_up_cache(buffer, bid, true);

          reprobe = 0;
          do {
            prefetch_info& info = buffer.front();
            const word*    w    = info.w;
            o                   = info.o;
            lo                  = info.lo;
            kvw                 = w + o->key.woff;
            nkey                = *kvw;
            if(nkey & o->key.lb_mask) {
              if(lo->key.mask2) {
                nkey = (nkey & lo->key.mask1 & ~lo->key.sb_mask1) >> lo->key.boff;
                nkey |=  ((*(kvw+1)) & lo->key.mask2 & ~lo->key.sb_mask2) << lo->key.shift;
              } else {
                nkey = (nkey & lo->key.mask1) >> lo->key.boff;
              }
              if(nkey == reprobe) {
                kvw = w + lo->val.woff;
                nval = ((*kvw) & lo->val.mask1) >> lo->val.boff;
                if(lo->val.mask2)
                  nval |= ((*(kvw+1)) & lo->val.mask2) << lo->val.shift;
                if(carry_bit) {
                  nval >>= 1;
                  val |= nval << (offsets.get_val_len() - 1);
                } else
                  val |= nval << offsets.get_val_len();
                break; // Should break only if carry_bit of nval is
                       // not set. Otherwise, we should reset the
                       // reprobe to 0 and try again.
              }
            }

            cid = (bid + reprobes[++reprobe]) & size_mask;
            
            prefetch_next(buffer, bid, reprobe, true);
          } while(reprobe <= reprobe_limit.val());
        }

        return true;
      }
  
      /**
       * Use hash values as counters
       */
      inline bool add(word key, word val, word *oval = 0) {
        uint64_t hash = hash_matrix.times(key);
        return add_rec(hash & size_mask, (hash >> lsize) & key_mask,
                       val, false, oval);
      }

      

      /**
       * Use hash as a set.
       */
      inline bool add(word _key, bool *is_new) __attribute__((deprecated)) {
	size_t id;
        return set(_key, is_new, &id);
      }
      inline bool set(word _key, bool *is_new) {
	size_t id;
	return set(_key, is_new, &id);
      }
      bool add(word _key, bool *is_new, size_t *id)  __attribute__((deprecated)) {
        return set(_key, is_new, id);
      }
      bool set(word _key, bool *is_new, size_t *id) {
        const offset_t *ao;
        uint64_t hash = hash_matrix.times(_key);
        word *w;
        *id = hash & size_mask;
        return claim_key((hash >> lsize) & key_mask, is_new, id, false,
                         &ao, &w);
      }

      /**
       * Use hash as a map. This sets a value with the key. It is only
       * partially thread safe. I.e., multiple different key can be
       * added concurrently. On the other hand, the same key can not
       * be added at the same time by different thread: the value set
       * may not be correct.
       */
      inline bool map(word _key, word val) {
        bool   is_new;
        return map(_key, val, &is_new);
      }

      bool map(word _key, word val, bool* is_new) {
        uint64_t hash = hash_matrix.times(_key);
        return map_rec(hash & size_mask, (hash >> lsize) & key_mask, val, false, 
                       is_new);
      }

      void write_ary_header(std::ostream *out) const {
        hash_matrix.dump(out);
        hash_inverse_matrix.dump(out);
      }

      void write_raw(std::ostream *out) const {
        if(out->tellp() & 0x7) { // Make sure aligned
          std::string padding(0x8 - (out->tellp() & 0x7), '\0');
          out->write(padding.c_str(), padding.size());
        }
        out->write((char *)mem_block.get_ptr(), mem_block.get_size());
      }

    private:
      /* id is input/output. Equal to hash & size_maks on input. Equal
       * to actual id where key was set on output. key is already hash
       * shifted and masked to get higher bits. (>> lsize & key_mask)
       *
       * is_new is set on output to true if key did not exists in hash
       * before. *ao points to the actual offsets object.
       */
      bool claim_key(const word &key, bool *is_new, size_t *id, bool large,
                     const offset_t **_ao, word **_w) {
        uint_t		 reprobe     = 0;
        const offset_t	*o, *lo, *ao;
        word		*w, *kw, nkey;
        bool		 key_claimed = false;
        size_t		 cid         = *id;
        word		 akey        = large ? 0 :(key | ((word)1 << key_off));

        do {
          *_w  = w = offsets.get_word_offset(cid, &o, &lo, data);
          *_ao = ao = large ? lo : o;

          kw = w + ao->key.woff;

          if(ao->key.mask2) { // key split on two words
            nkey = akey << ao->key.boff;
            nkey |= ao->key.sb_mask1;
            if(large)
              nkey |= ao->key.lb_mask;
            nkey &= ao->key.mask1;

            // Use o->key.mask1 and not ao->key.mask1 as the first one is
            // guaranteed to be bigger. The key needs to be free on its
            // longer mask to claim it!
            key_claimed = set_key(kw, nkey, o->key.mask1, ao->key.mask1);
            if(key_claimed) {
              nkey = ((akey >> ao->key.shift) | ao->key.sb_mask2) & ao->key.mask2;
              key_claimed = key_claimed && set_key(kw + 1, nkey, o->key.mask2, ao->key.mask2, is_new);
            }
          } else { // key on one word
            nkey = akey << ao->key.boff;
            if(large)
              nkey |= ao->key.lb_mask;
            nkey &= ao->key.mask1;
            key_claimed = set_key(kw, nkey, o->key.mask1, ao->key.mask1, is_new);
          }
          if(!key_claimed) { // reprobe
            if(++reprobe > reprobe_limit.val())
              return false;
            cid = (*id + reprobes[reprobe]) & size_mask;

            if(large)
              akey = reprobe;
            else
              akey = key | ((reprobe + 1) << key_off);
          }
        } while(!key_claimed);

        *id = cid;
        return true;
      }

      bool add_rec(size_t id, word key, word val, bool large, word *oval) {
        const offset_t	*ao;
        word		*w;

        bool is_new = false;
        if(!claim_key(key, &is_new, &id, large, &ao, &w))
          return false;
        if(oval)
          *oval = !is_new;

        // Increment value
        word *vw = w + ao->val.woff;
        word cary = add_val(vw, val, ao->val.boff, ao->val.mask1);
        cary >>= ao->val.shift;
        if(cary && ao->val.mask2) { // value split on two words
          cary = add_val(vw + 1, cary, 0, ao->val.mask2);
          cary >>= ao->val.cshift;
        }
        if(cary) {
          id = (id + reprobes[0]) & size_mask;
          if(add_rec(id, key, cary, true, 0))
            return true;

          // Adding failed, table is full. Need to back-track and
          // substract val.
          cary = add_val(vw, ((word)1 << offsets.get_val_len()) - val,
                         ao->val.boff, ao->val.mask1);
          cary >>= ao->val.shift;
          if(cary && ao->val.mask2) {
            // Can I ignore the cary here? Table is known to be full, so
            // not much of a choice. But does it leave the table in a
            // consistent state?
            add_val(vw + 1, cary, 0, ao->val.mask2);
          }
          return false;
        }
        return true;
      }

      // Store val in the hash at position id. Reprobe and recurse if
      // val does not fit in counter field. A bit is added at the
      // beginning of the counting field indicating whether there is
      // another entry for the same key further in the hash.
      bool map_rec(size_t id, word key, word val, bool large, bool* is_new) {
        const offset_t* ao;
        word*           w;
        bool            is_new_entry = false;
        if(!claim_key(key, &is_new_entry, &id, large, &ao, &w))
          return false;
        if(is_new)
          *is_new = is_new_entry;

        // Determine if there will be a carry
        val <<= 1;
        val |= (val > offsets.get_max_val(large));

        // Set value
        word *vw         = w + ao->val.woff;
        word  oval       = 0;
        word  cary       = set_val(vw, val, ao->val.boff, ao->val.mask1, oval);
        cary           >>= ao->val.shift;
        bool  cary_bit   = oval & 0x1;
        if(ao->val.mask2) { // value split on two words. Write even if
                            // cary is 0 as there maybe some value in
                            // there already
          cary   = set_val(vw + 1, cary, 0, ao->val.mask2, oval);
          cary >>= ao->val.cshift;
        }
        // Done if there is no carry and previous value did not have
        // the carry_bit set
        if(!cary && !cary_bit)
          return true;
        id = (id + reprobes[0]) & size_mask;
        return map_rec(id, key, cary, true, 0);
      }

      inline bool set_key(word *w, word nkey, word free_mask, 
                          word equal_mask) {
        word ow = *w, nw, okey;

        okey = ow & free_mask;
        while(okey == 0) { // large bit not set && key is free
          nw = atomic.cas(w, ow, ow | nkey);
          if(nw == ow)
            return true;
          ow = nw;
          okey = ow & free_mask;
        }
        return (ow & equal_mask) == nkey;
      }

      inline bool set_key(word *w, word nkey, word free_mask, 
                          word equal_mask, bool *is_new) {
        word ow = *w, nw, okey;

        okey = ow & free_mask;
        while(okey == 0) { // large bit not set && key is free
          nw = atomic.cas(w, ow, ow | nkey);
          if(nw == ow) {
            *is_new = true;
            return true;
          }
          ow = nw;
          okey = ow & free_mask;
        }
        *is_new = false;
        return (ow & equal_mask) == nkey;
      }


      inline word add_val(word *w, word val, uint_t shift, word mask) {
        word now = *w, ow, nw, nval;

        do {
          ow = now;
          nval = ((ow & mask) >> shift) + val;
          nw = (ow & ~mask) | ((nval << shift) & mask);
          now = atomic.cas(w, ow, nw);
        } while(now != ow);

        return nval & (~(mask >> shift));
      }

      inline word set_val(word *w, word val, uint_t shift, word mask,
                          word& oval) {
        word now = *w, ow, nw;
        word sval = (val << shift) & mask;

        do {
          ow = now;
          nw = (ow & ~mask) | sval;
          now = atomic.cas(w, ow, nw);
        } while(now != ow);

        oval = (ow & mask) >> shift;
        return val & (~(mask >> shift));
      }
    };

    /*****/
  }
}

#endif // __REVERSIBLE_HASH__
