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

#ifndef __JELLYFISH_SMALL_PACKED_ARRAY__
#define __JELLYFISH_SMALL_PACKED_ARRAY__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <iostream>

#define PRINTVAR(v) {std::cout << __LINE__ << " " #v ": " << v << std::endl; }

/* A chunck is a smallest block of aligned 64 bits words which contain
 * a whole number of items (including the extra bits to mark an entry
 * as used). chunck_words is the number of 64 bits words in a chunck.
 *
 * A value which is contained within one aligned 64 bits word has only
 * 1 bit used to mark the entry as used (lowest weight bit). A value
 * which spans over 2 words uses 2 bits. (lowest weight bit in each
 * word).
 *
 * chunck_items is the number of items in a chunck. chunck_items and
 * chunck_words satisfy the equations (with k = chunck_items; s =
 * item_len; w = chunck_words and w minimized):
 *
 * k * (s/64 + s + 1) + w - 1 = 64 * w
 *
 * If (s/w + s + 1) is not prime with 63 (i.e. it is a multiple of 3
 * or 7), this has no solution, and 1 extra bit per chunk will be
 * lost. The relation is then
 *
 * k * (s/64 + s + 1) + w = 64 * w
 *
 * At first, assume that s/64 is 0 (i.e. s <= 63) 
 */
class small_packed_array {
  uint64_t      *data;
  size_t        data_len;
  uint32_t      item_len;
  uint64_t      size;

  uint32_t      chunck_words;
  uint32_t      chunck_items;
  uint64_t      item_mask;

  bool          allocated;

  void initialize()
  {
    uint32_t used_bits = item_len + 1;
    chunck_words = 1;
    while(1) {
      if((63 * chunck_words + 1) % used_bits == 0) {
        chunck_items = (63 * chunck_words + 1) / used_bits;
        break;
      }
      if((63 * chunck_words) % used_bits == 0) {
        chunck_items = (63 * chunck_words) / used_bits;
        break;
      }
      chunck_words++;
    }
    data_len = 8 * chunck_words * (size / chunck_items);
    if(size % chunck_items)
      data_len += 8 * chunck_words;
  }

public:
  small_packed_array(uint32_t _item_len, uint64_t _size) :
    item_len(_item_len), size(_size), item_mask((UINT64_C(1) << _item_len) - 1),
    allocated(true)
  {
    initialize();
    data = (uint64_t *)malloc(data_len);
    memset(data, '\0', data_len);
  }
  
  small_packed_array(char *map, uint32_t _item_len, uint64_t _size) :
    item_len(_item_len), size(_size), item_mask((UINT64_C(1) << _item_len) - 1),
    allocated(false)
  {
    initialize();
    data = (uint64_t *)map;
  }
  
  
  ~small_packed_array() {
    if(allocated)
      free(data);
  }

  void write(std::ostream &out) {
    out.write((char *)data, data_len);
  }
  
  inline bool __set(uint64_t *word, uint64_t mask, uint32_t off, uint64_t val)
  {
    uint64_t oword, nword, eval;

    oword = *word;
    while(1) {
      eval = (oword >> off);
      if(eval & 0x1)
        return ((eval >> 1) & mask) == (val & mask); // val already in
      nword = oword | (((val & mask) << 1 | UINT64_C(0x1)) << off);
      nword = __sync_val_compare_and_swap(word, oword, nword);
      if(oword == nword)
        return true;    // Successfully added val
      oword = nword;
    }
  }

  /* Set entry idx to val if entry is empty.
   * Return true if set successful or entry is equal to val.
   */
  bool set(uint64_t idx, uint64_t val)
  {
    uint32_t cidx = idx % chunck_items;
    uint32_t widx = (cidx * (item_len + 1)) / 63;
    uint64_t *word = data + chunck_words * (idx / chunck_items) + widx;
    uint64_t mask;
    uint32_t off = (item_len + 1) * cidx - 63 * widx;

    if(off + item_len < 64) {
      // Contain within one word
      return __set(word, item_mask, off, val);
    } else {
      mask = (UINT64_C(1) << (63 - off)) - 1;
      if(__set(word, mask, off, val)) {
        mask = item_mask >> (63 - off);
        return __set(word+1, mask, 0, val >> (63 - off));
      } else {
        return false;
      }
    }
  }

  inline bool __get(uint64_t *word, uint64_t mask, uint32_t off, uint64_t *val)
  {
    uint64_t eval = *word >> off;

   if(eval & 0x1) {
      *val = (eval >> 1) & mask;
      return true;
    }
    return false;
  }

  /* Get the entry idx into val. Return true if entry is set, false
   * otherwise.
   */
  bool get(uint64_t idx, uint64_t *val)
  {
    uint32_t cidx = idx % chunck_items;
    uint32_t widx = (cidx * (item_len + 1)) / 63;
    uint64_t *word = data + chunck_words * (idx / chunck_items) + widx;
    uint64_t mask, ival;
    uint32_t off = (item_len + 1) * cidx - 63 * widx;

    
    if(off + item_len < 64) {
      return __get(word, item_mask, off, val);
    } else {
      mask = (UINT64_C(1) << (63 - off)) - 1;
      if(__get(word, mask, off, &ival)) {
        *val = ival;
        mask = item_mask >> (63 - off);
        if(__get(word + 1, mask, 0, &ival)) {
          *val |= ival << (63 - off);
          return true;
        }
      }
      return false;
    }
  }
  
  inline uint64_t *get_data() { return data; }
  inline size_t get_data_len() { return data_len; }
};
#endif // __SMALL_PACKED_ARRAY__
