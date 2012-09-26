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

#ifndef __JELLYFISH_OFFSETS_KEY_VALUE_HPP__
#define __JELLYFISH_OFFSETS_KEY_VALUE_HPP__

#include <jellyfish/misc.hpp>
#include <jellyfish/divisor.hpp>
#include <signal.h>

namespace jellyfish {
  /* A word is whatever aligned type used for atomic operations
   * (CAS). Typically, a uint64_t. We store pairs of (key, value), in a
   * bit packed fashion. The key and value can have abritrary size as
   * long as they each fit in one word. A block is the largest number of
   * (key, value) pair such that the first key, and only the first,
   * starts at an aligned word.
   *
   * The key 0x0 is not valid. A key which fits completely within one
   * word is not protected by a "set" bit. A key which straddle the
   * boundary between two aligned words has a set bit in each parts.
   *
   * A value field can have any value and is initialized to 0x0. It has
   * no "set" bit ever.
   *
   * A key is prefixed with a "large" bit. If this bit is 0, the key
   * field is length key_len (not counting the possible set bits) and
   * the value field has length val_len. If the large bit has value 1,
   * the key field is just long enough to encode the number of
   * reprobing hops to go backward to find the actual key. The
   * remainder bits is used for the value field. In this scheme, we
   * assume the length needed to encode the number of reprobes is much
   * less than the length needed to encode the key.
   */

  /* Offsets holds all the possible offset for a given combination of
   * key length, value length and reprobe limit.
   */
  template<typename word>
  class Offsets {
  public:
    // woff: offset in words from beginning of block
    // boff: offset in bits within that word. Past large bit.
    // shift: shift in second word, if any
    // mask1: includes the large bit and the set bit if any.
    // mask2: mask in second word. Idem as mask1. 0 if fits in one word.
    // sb_mask[12]: mask for set bit in word 1 and 2, if any. set bit is the
    //              last usable bit of the field.
    // lb_mask: mask for the large bit. It is the first bit of the key field.
    typedef struct {
      struct {
        uint_t woff, boff, shift, cshift;
        word   mask1, mask2, sb_mask1, sb_mask2, lb_mask;
      } key;
      struct {
        uint_t woff, boff, shift, cshift;
        word   mask1, mask2;
      } val;
    } offset_t;
    typedef struct {
      offset_t    normal;
      offset_t    large;
    } offset_pair_t;
    struct block_info {
      uint_t len;
      uint_t word_len;
    };
    //    Offsets() {}

    Offsets(uint_t _key_len, uint_t _val_len, uint_t _reprobe_limit) :
      key_len(_key_len),
      val_len(_val_len),
      reprobe_limit(_reprobe_limit),
      reprobe_len(bitsize(reprobe_limit)),
      lval_len(key_len + val_len - reprobe_len),
      block(compute_offsets()),
      bld(block.len)
    { }

    ~Offsets() {}

    uint_t get_block_len() const { return block.len; }
    uint_t get_block_word_len() const { return block.word_len; }
    uint_t get_reprobe_len() const { return reprobe_len; }
    uint_t get_key_len() const { return key_len; }
    uint_t get_val_len() const { return val_len; }
    uint_t get_lval_len() const { return lval_len; }
    word   get_max_val(bool large) const { 
      return (((uint64_t)1) << (large ? lval_len : val_len)) - 1;
    }

    // Discretize and round down number of entries according to length
    // of a block. Return in blocks the number of blocks.
    size_t floor_block(size_t entries, size_t &blocks) const {
      blocks = entries / bld;
      return block.len * blocks;
    }

    word *get_word_offset(size_t id, const offset_t **o, const offset_t **lo,
			  word * const base) const {
      uint64_t q, r;
      bld.division(id, q, r);
      word *w = base + (block.word_len * q);
      *o = &offsets[r].normal;
      *lo = &offsets[r].large;
      return w;
    }

  private:
    const uint_t     key_len, val_len;
    const uint_t     reprobe_limit, reprobe_len, lval_len;
    const block_info block;
    const divisor64  bld;       // Fast divisor by block.len
    offset_pair_t    offsets[bsizeof(word)];

    block_info compute_offsets();
    bool update_current_offsets(uint_t &cword, uint_t &cboff, uint_t add);
    word mask(uint_t length, uint_t shift);
  };

  template<typename word>
  bool Offsets<word>::update_current_offsets(uint_t &cword, uint_t &cboff, uint_t add)
  {
    cboff += add;
    if(cboff >= bsizeof(word)) {
      cword++;
      cboff %= bsizeof(word);
      return cboff > 0;
    }
    return false; 
  }

  template<typename word>
  word Offsets<word>::mask(uint_t length, uint_t shift)
  {
    return (((word)1u << length) - 1) << shift;
  }

  template<typename word>
  typename Offsets<word>::block_info Offsets<word>::compute_offsets()
  {
    offset_pair_t *offset = offsets;
    uint_t         cword  = 0;    // current word in block
    uint_t         cboff  = 0;    // current offset in word
    uint_t         lcword;        // idem for large fields
    uint_t         lcboff;
    uint_t         ocboff;

    memset(offsets, '\0', sizeof(offsets));
    do {
      offset->normal.key.woff    = offset->large.key.woff = lcword = cword;
      ocboff                     = lcboff = cboff;
      offset->normal.key.boff    = cboff + 1;
      offset->normal.key.lb_mask = mask(1, cboff);
      if(update_current_offsets(cword, cboff, key_len + 1)) {
        // key extends over two words -> add extra set bits
        update_current_offsets(cword, cboff, 2);
        offset->normal.key.mask1    = mask(bsizeof(word) - ocboff, ocboff);
        offset->normal.key.mask2    = mask(cboff, 0);
        offset->normal.key.shift    = key_len + 1 - cboff;
        offset->normal.key.cshift   = cboff - 1;
        offset->normal.key.sb_mask1 = mask(1, bsizeof(word) - 1);
        offset->normal.key.sb_mask2 = mask(1, cboff - 1);
      } else {
        offset->normal.key.mask1    = mask(key_len + 1, ocboff);
        offset->normal.key.mask2    = 0;
        offset->normal.key.shift    = 0;
        offset->normal.key.cshift   = 0;
        offset->normal.key.sb_mask1 = 0;
        offset->normal.key.sb_mask2 = 0;
      }
      offset->normal.val.woff  = cword;
      offset->normal.val.boff  = cboff;
      offset->normal.val.mask1 = mask(val_len, cboff);
      if(update_current_offsets(cword, cboff, val_len)) {
        offset->normal.val.mask2  = mask(cboff, 0);
        offset->normal.val.shift  = val_len - cboff;
        offset->normal.val.cshift = cboff;
      } else {
        offset->normal.val.mask2  = 0;
        offset->normal.val.shift  = val_len;
        offset->normal.val.cshift = 0;
      }

      ocboff                    = lcboff;
      offset->large.key.boff    = lcboff + 1;
      offset->large.key.lb_mask = mask(1, lcboff);
      if(update_current_offsets(lcword, lcboff, reprobe_len + 1)) {
        update_current_offsets(lcword, lcboff, 2);
        offset->large.key.mask1    = mask(bsizeof(word) - ocboff, ocboff);
        offset->large.key.mask2    = mask(lcboff, 0);
        offset->large.key.shift    = reprobe_len + 1 - lcboff;
        offset->large.key.cshift   = lcboff - 1;
        offset->large.key.sb_mask1 = mask(1, bsizeof(word) - 1);
        offset->large.key.sb_mask2 = mask(1, lcboff - 1);
      } else {
        offset->large.key.mask1    = mask(reprobe_len + 1, ocboff);
        offset->large.key.mask2    = 0;
        offset->large.key.boff     = ocboff + 1;
        offset->large.key.shift    = 0;
        offset->large.key.cshift   = 0;
        offset->large.key.sb_mask1 = 0;
        offset->large.key.sb_mask2 = 0;
      }
      offset->large.val.woff  = lcword;
      offset->large.val.boff  = lcboff;
      offset->large.val.mask1 = mask(lval_len, lcboff);
      if(update_current_offsets(lcword, lcboff, lval_len)) {
        offset->large.val.mask2  = mask(lcboff, 0);
        offset->large.val.shift  = lval_len - lcboff;
        offset->large.val.cshift = lcboff;
      } else {
        offset->large.val.mask2  = 0;
        offset->large.val.shift  = lval_len;
        offset->large.val.cshift = 0;
      }

      offset++;
    } while(cboff != 0 && cboff < bsizeof(word) - 2);

    block_info res = { static_cast<uint_t>(offset - offsets), cword + (cboff == 0 ? 0 : 1) };
    return res;
  }
}

#endif // __OFFSETS_KEY_VALUE_HPP__
