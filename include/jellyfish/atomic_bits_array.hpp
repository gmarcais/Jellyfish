/* Quorum
 * Copyright (C) 2012  Genome group at University of Maryland.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __JELLYFISH_ATOMIC_BITS_ARRAY_HPP__
#define __JELLYFISH_ATOMIC_BITS_ARRAY_HPP__

#include <stdexcept>

#include <jellyfish/allocators_mmap.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/divisor.hpp>

namespace jellyfish {
template<typename Value, typename T = uint64_t>
class atomic_bits_array {
  static const int       w_ = sizeof(T) * 8;
  const int              bits_;
  const size_t           size_;
  const T                mask_;
  const jflib::divisor64 d_;
  allocators::mmap       mem_;
  T*                     data_;
  static atomic::gcc     atomic_;

  class element_proxy {
    T*        word_;
    const T   mask_;
    const int off_;
    T         prev_word_;

    Value get_val() const {
      return static_cast<Value>((prev_word_ & mask_) >> off_);
    }

  public:
    element_proxy(T* word, T mask, int off) :
      word_(word), mask_(mask), off_(off)
    { }

    Value get() {
      prev_word_ = *word_;
      return get_val();
    }

    bool set(Value& nval) {
      const T new_word    = (prev_word_ & ~mask_) | ((static_cast<T>(nval) << off_) & mask_);
      const T actual_word = atomic_.cas(word_, prev_word_, new_word);
      if(__builtin_expect(actual_word == prev_word_, 1))
        return true;
      prev_word_ = actual_word;
      nval       = get_val();
      return false;
    }
  };

public:
  atomic_bits_array(int bits, // Number of bits per entry
                    size_t size) : // Number of entries
    bits_(bits),
    size_(size),
    mask_((T)-1 >> (w_ - bits)), // mask of one entry at the LSB of a word
    d_(w_ / bits),              // divisor of the number of entries per word
    mem_(size / d_ + (size % d_ != 0)),
    data_((T*)mem_.get_ptr())
  {
    static_assert(sizeof(T) >= sizeof(Value), "Container type T must have at least as many bits as value type");
    if(bits > sizeof(Value) * 8)
      throw std::runtime_error("The number of bits per entry must be less than the number of bits in the value type");
    if(!data_)
      throw std::runtime_error("Can't allocate memory for atomic_bits_array");
  }

  // Return the element at position pos. No check for out of bounds.
  element_proxy operator[](size_t pos) {
    uint64_t q, r;
    d_.division(pos, q, r);
    const int off = r * bits_;
    return element_proxy(data_ + q, mask_ << off, off);
  }
};
} // namespace jellyfish

#endif /* __JELLYFISH_ATOMIC_BITS_ARRAY_HPP__ */
