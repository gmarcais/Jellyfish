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

#ifndef __JELLYFISH_LARGE_HASH_ARRAY_HPP__
#define __JELLYFISH_LARGE_HASH_ARRAY_HPP__

#include <jellyfish/storage.hpp>
#include <jellyfish/offsets_key_value.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/rectangular_binary_matrix.hpp>

namespace jellyfish { namespace large_hash {
/* Contains an integer, the reprobe limit. It is capped based on the
 * reprobe strategy to not be bigger than the size of the hash
 * array. Also, the length to encode reprobe limit must not be larger
 * than the length to encode _size.
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

// Key is any type with the following two methods: get_bits(unsigned
// int start, unsigned int len); and set_bits(unsigned int start,
// unsigned int len, uint64_t bits). These methods get and set the
// bits [start, start + len). Start and len may not be aligned to word
// boundaries. On the other hand, len is guaranteed to be <
// sizeof(uint64_t). I.e. never more than 1 word is fetched or set.
template<typename Key, typename word = uint64_t, typename atomic_t = atomic::gcc, typename mem_block_t = allocators::mmap>
class array {
  define_error_class(ErrorAllocation);

  static const int  wsize = std::numeric_limits<word>::digits; // Word size in bits
  // Can't be done. Resort to an evil macro!
  //  static const word fmask = std::numeric_limits<word>::max(); // Mask full of ones
#define fmask (std::numeric_limits<word>::max())

  typedef typename Offsets<word>::offset_t offset_t;
  uint16_t                 lsize; // log of size
  size_t                   size, size_mask;
  reprobe_limit_t          reprobe_limit;
  uint16_t                 key_len; // Length of key in bits
  uint16_t                 raw_key_len; // Length of key stored raw (i.e. complement of implied length)
  Offsets<word>            offsets; // key len reduced by size of hash array
  mem_block_t              mem_block;
  word                    *data;
  atomic_t                 atomic;
  size_t                  *reprobes;
  RectangularBinaryMatrix  hash_matrix;
  RectangularBinaryMatrix  hash_inverse_matrix;


public:
  typedef Key               key_type;
  typedef uint64_t          mapped_type;
  typedef std::pair<const   Key, uint64_t> value_type;
  // TODO: iterators
  typedef value_type&       reference;
  typedef const value_type& const_reference;
  typedef value_type*       pointer;
  typedef const value_type* const_pointer;

  array(size_t size_, // Size of hash. To be rounded up to a power of 2
        uint16_t key_len_, // Size of key in bits
        uint16_t val_len_, // Size of val in bits
        uint16_t reprobe_limit_, // Maximum reprobe
        size_t* reprobes_ = jellyfish::quadratic_reprobes) : // Reprobing policy
    lsize(ceilLog2(size_)),
    size((size_t)1 << lsize),
    size_mask(size - 1),
    reprobe_limit(reprobe_limit_, reprobes_, size),
    key_len(key_len_),
    raw_key_len(key_len - lsize),
    offsets(key_len - lsize + bitsize(reprobe_limit.val() + 1), val_len_, reprobe_limit.val() + 1),
    mem_block(div_ceil(size, (size_t)offsets.get_block_len()) * offsets.get_block_word_len() * sizeof(word)),
    data((word *)mem_block.get_ptr()),
    reprobes(reprobes_),
    hash_matrix(lsize, key_len),
    hash_inverse_matrix(hash_matrix.randomize_pseudo_inverse(random_bits))
  {
    if(!data)
      eraise(ErrorAllocation) << "Failed to allocate "
                              << (div_ceil(size, (size_t)offsets.get_block_len()) * offsets.get_block_word_len() * sizeof(word))
                              << " bytes of memory";
  }

  size_t get_size() const { return size; }
  size_t get_lsize() const { return lsize; }
  uint_t get_key_len() const { return key_len; }
  uint_t get_val_len() const { return offsets.get_val_len(); }

  uint_t get_max_reprobe() const { return reprobe_limit.val(); }
  size_t get_max_reprobe_offset() const {
    return reprobes[reprobe_limit.val()];
  }

  RectangularBinaryMatrix get_matrix() const { return hash_matrix; }
  RectangularBinaryMatrix get_inverse_matrix() const { return hash_inverse_matrix; }

  /**
   * Clear hash table. Not thread safe.
   */
  void clear() {
    memset(data, '\0', mem_block.get_size());
  }

  /**
   * Use hash values as counters.
   *
   * The matrix multiplication gets only a uint64_t. The lsb of the
   * matrix product, the hsb are assume to be equal to the key itself
   * (the matrix has a partial identity on the first rows).
   */
  inline bool add(const key_type&key, mapped_type val, bool* is_new, size_t* id) {
    uint64_t hash = hash_matrix.times(key);
    return add_rec(hash & size_mask, key, val, false, is_new, id);
  }
  inline bool add(const key_type& key, mapped_type val) {
    bool is_new;
    size_t id;
    return add(key, val, &is_new, &id);
  }

  //////////////////////////////
  // Iterator
  //////////////////////////////
  class iterator {
  protected:
    const array	*ary_;
    size_t	 start_id_, id_, end_id_;

    key_type    key_;
    mapped_type val_;

  public:
    iterator(const array *ary, size_t start, size_t end) :
      ary_(ary),
      start_id_(start > ary->get_size() ? ary->get_size() : start),
      id_(start),
      end_id_(end > ary->get_size() ? ary->get_size() : end)
    {}

    uint64_t start() const { return start_id_; }
    uint64_t end() const { return end_id_; }
    const key_type& key() const { return key_; }
    const mapped_type& val() const { return val_; }
    size_t id() const { return id_ - 1; }

    bool next() {
      bool success = false;
      while(!success && id < end_id_)
        success = ary_->get_key_val_at_id(id_++, key, val);

      return success;
    }
  };
  friend class iterator;
  iterator iterator_all() const { return iterator(this, 0, get_size()); }
  iterator iterator_slice(size_t slice_number, size_t number_of_slice) const {
    std::pair<size_t, size_t> res = slice(slice_number, number_of_slice, get_size());
    return iterator(this, res.first, res.second);
  }

protected:
    /* id is input/output. Equal to hash & size_maks on input. Equal
   * to actual id where key was set on output. key is already hash
   * shifted and masked to get higher bits. (>> lsize & key_mask)
   *
   * is_new is set on output to true if key did not exists in hash
   * before. *ao points to the actual offsets object and w to the word
   * holding the value.
   */
  bool claim_key(const key_type& key, bool* is_new, size_t* id, bool large,
                 const offset_t** _ao, word** _w) {
    uint_t	    reprobe        = 0;
    const offset_t *o, *lo, *ao;
    word	   *w, *kw, nkey;
    bool	    key_claimed    = false;
    size_t	    cid            = *id;
    int             bits_copied    = lsize; // Bits from original key already copied, explicitly or implicitly

    // Akey contains first word of what to store in the key
    // field. I.e. part of the original key (the rest is encoded in
    // the original position) and the reprobe value to substract from
    // the actual position to get to the original position.
    //
    //    MSB                     LSB
    //   +--------------+-------------+
    //   |  MSB of key  |  reprobe    |
    //   + -------------+-------------+
    //     raw_key_len    reprobe_len
    //
    // Akey is updated at every operation to reflect the current
    // reprobe value. nkey is the temporary word containing the part
    // to be stored in the current word kw (+ some offset).
    word akey = 0;
    if(!large) {
      akey         = 1;         // start reprobe value == 0. Store reprobe value + 1
      int to_copy  = std::min((int)(wsize - offsets.get_reprobe_len()), key_len - bits_copied);
      akey        |= key.get_bits(bits_copied, to_copy) << offsets.get_reprobe_len();
      bits_copied += to_copy;
    }

    do {
      *_w  = w = offsets.get_word_offset(cid, &o, &lo, data);
      *_ao = ao = large ? lo : o;

      kw = w + ao->key.woff;

      if(ao->key.sb_mask1) { // key split on multiple words
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
          nkey = akey >> ao->key.shift;
          if(bits_copied + wsize - 1 < key_len) {
            // Copy full words. First one is special 
            nkey                  |= key.get_bits(bits_copied, ao->key.shift - 1) << (wsize - ao->key.shift);
            nkey                  |= ao->key.sb_mask1; // Set bit is MSB
            bits_copied           += ao->key.shift - 1;
            int copied_full_words  = 1;
            key_claimed            = set_key(kw + copied_full_words, nkey, fmask, fmask);
            // Copy more full words if needed
            while(bits_copied + wsize - 1 < key_len && key_claimed) {
              nkey               = key.get_bits(bits_copied, wsize - 1);
              nkey              |= ao->key.sb_mask1;
              bits_copied       += wsize - 1;
              copied_full_words += 1;
              key_claimed        = set_key(kw + copied_full_words, nkey, fmask, fmask);
              bits_copied       += wsize - 1;
            }
            assert((bits_copied < key_len) == (ao->key.sb_mask2 != 0));
            if(ao->key.sb_mask2 && key_claimed) { // Copy last word
              nkey               = key.get_bits(bits_copied, key_len - bits_copied);
              nkey              |= ao->key.sb_mask2;
              copied_full_words += 1;
              key_claimed        = set_key(kw + copied_full_words, nkey, o->key.mask2, ao->key.mask2, is_new);
            }
          } else if(ao->key.sb_mask2) { // if bits_copied + wsize - 1 < key_len
            // Copy last word, no full words copied
            nkey |= key.get_bits(bits_copied, key_len - bits_copied) << (wsize - ao->key.shift);
            nkey |= ao->key.sb_mask2;
            nkey &= ao->key.mask2;
            key_claimed = key_claimed && set_key(kw + 1, nkey, o->key.mask2, ao->key.mask2, is_new);
          }
        } // if(key_claimed)
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
          akey = 0; // TODO: msb_hash_key | ((reprobe + 1) << key_off);
      }
    } while(!key_claimed);

    *id = cid;
    return true;
  }

  // Add val to key. id is the starting place (result of hash
  // computation). eid is set to the effective place in the
  // array. large is set to true is setting a large key (upon
  // recurrence if there is a carry).
  bool add_rec(size_t id, const key_type& key, word val, bool large, bool* is_new, size_t* eid) {
    const offset_t	*ao;
    word		*w;

    if(!claim_key(key, is_new, &id, large, &ao, &w))
      return false;
    *eid = id;

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
      size_t ignore_eid;
      bool   ignore_is_new;
      if(add_rec(id, key, cary, true, &ignore_is_new, &ignore_eid))
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

  // Atomic methods to set the key. Attempt to set nkey in word w. All
  // bits matching free_mask must be unset and the bits matching
  // equal_mask must be equal for a success in setting the key. Set
  // is_new to true if the spot was previously empty. Otherwise, if
  // is_new is false but true is returned, the key was already present
  // at that spot.
  inline bool set_key(word *w, word nkey, word free_mask, word equal_mask, bool *is_new) {
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

  inline bool set_key(word *w, word nkey, word free_mask, word equal_mask) {
    bool is_new;
    return set_key(w, nkey, free_mask, equal_mask, &is_new);
  }

  // Add val the value in word w, with shift and mask giving the
  // particular part of the word in which the value is stored. The
  // return value is the carry.
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

  // Return the key and value at position id. If the slot at id is
  // empty or has the large bit set, returns false. Otherwise, returns
  // the key and the value is the sum of all the entries in the hash
  // table for that key. I.e., the table is search forward for entries
  // with large bit set pointing back to the key at id, and all those
  // values are summed up.
  bool get_key_val_at_id(size_t id, mer_dna& key, word& val,
                         bool carry_bit = false) const {
    const offset_t	*o, *lo;
    word		*w, *kvw, key_word, kreprobe = 0;
    size_t		 cid;

    w   = offsets.get_word_offset(id, &o, &lo, data);
    kvw = w + o->key.woff;
    key_word = *kvw;
    if(key_word & o->key.lb_mask)
      return false;
    int bits_copied = lsize;
    if(o->key.sb_mask1) {
      if((key_word & o->key.sb_mask1) == 0)
        return false;
      kreprobe = (key_word & o->key.mask1 & ~o->key.sb_mask1) >> o->key.boff;
      if(bits_copied + wsize - 1 < key_len) {
        // Copy full words. First one is special
        key_word = *(kvw + 1);
        if(offsets.get_reprobe_len() < o->key.shift) {
          key.set_bits(bits_copied, o->key.shift - offsets.get_reprobe_len(), kreprobe >> offsets.get_reprobe_len());
          bits_copied += o->key.shift - offsets.get_reprobe_len();
          kreprobe &= offsets.get_reprobe_mask();
          key.set_bits(bits_copied, wsize - 1, key_word & ~o->key.sb_mask1);
        } else {
          int reprobe_left  = offsets.get_reprobe_len() - o->key.shift;
          kreprobe         |= (key_word & (((word)1 << reprobe_left) - 1)) << o->key.shift;
          key.set_bits(bits_copied, wsize - 1 - reprobe_left, (key_word & ~o->key.sb_mask1) >> reprobe_left);
        }
        int word_copied = 2;
        while(bits_copied + wsize - 1 < key_len) {
          key.set_bits(bits_copied, wsize - 1, *(kvw + word_copied++) & (fmask >> 1));
          bits_copied += wsize - 1;
        }
        if(o->key.sb_mask2)
          key.set_bits(bits_copied, key_len - bits_copied, *(kvw + word_copied) & o->key.mask2 & ~o->key.sb_mask2);
      } else if(o->key.sb_mask2) { // if(bits_copied + wsize - 1 < key_len
        // Two words but no full words
        key_word = *(kvw + 1) & o->key.mask2 & ~o->key.sb_mask2;
        if(offsets.get_reprobe_len() < o->key.shift) {
          key.set_bits(bits_copied, o->key.shift - offsets.get_reprobe_len(), kreprobe >> offsets.get_reprobe_len());
          bits_copied += o->key.shift - offsets.get_reprobe_len();
          kreprobe    &= offsets.get_reprobe_mask();
          key.set_bits(bits_copied, key_len - bits_copied, key_word);
        } else {
          int reprobe_left  = offsets.get_reprobe_len() - o->key.shift;
          kreprobe         |= (key_word & (((word)1 << reprobe_left) - 1)) << o->key.shift;
          key.set_bits(bits_copied, key_len - bits_copied, key_word >> reprobe_left);
        }
      }
    } else { // if(o->key.sb_mask1
      // Everything in 1 word
      key_word = (key_word & o->key.mask1) >> o->key.boff;
      if(key_word == 0)
        return false;
      kreprobe = key_word & offsets.get_reprobe_mask();
      key.set_bits(bits_copied, raw_key_len, key_word >> offsets.get_reprobe_len());
    }
    // Compute missing part of key with inverse matrix
    key.set_bits(0, lsize, (id - (kreprobe - 1)) & size_mask);
    key.set_bits(0, lsize, hash_inverse_matrix.times(key));

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
    uint_t overflows = 0, reprobe = 0;
    cid = id = (id + reprobes[0]) & size_mask;
    while(reprobe <= reprobe_limit.val()) {
      if(reprobe)
        cid  = (id + reprobes[reprobe]) & size_mask;

      w    = offsets.get_word_offset(cid, &o, &lo, data);
      kvw  = w + o->key.woff;
      word nkey = *kvw;
      if(nkey & o->key.lb_mask) {
        // If the large bit is set, the size of the key (reprobe_len)
        // is guaranteed to have a length of at most 1 word.
        if(lo->key.sb_mask1) {
          nkey  = (nkey & lo->key.mask1 & ~lo->key.sb_mask1) >> lo->key.boff;
          nkey |= ((*(kvw+1)) & lo->key.mask2 & ~lo->key.sb_mask2) << lo->key.shift;
        } else {
          nkey = (nkey & lo->key.mask1) >> lo->key.boff;
        }
        if(nkey == reprobe) {
          kvw = w + lo->val.woff;
          word nval = ((*kvw) & lo->val.mask1) >> lo->val.boff;
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
        if(o->key.sb_mask1) {
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

};

} } // namespace jellyfish { namespace large_hash_array

#endif /* __JELLYFISH_LARGE_HASH_ARRAY_HPP__ */
