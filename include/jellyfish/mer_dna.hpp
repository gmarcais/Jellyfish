/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <string.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <limits>
#include <iterator>
#include <random>
#include <type_traits>
#include <utility>

#ifdef HAVE_INT128
#include <jellyfish/int128.hpp>
#endif

namespace jellyfish { namespace mer_dna_ns {
#define R -1
#define I -2
#define O -3
#define A 0
#define C 1
#define G 2
#define T 3
static const int codes[256] = {
  O, O, O, O, O, O, O, O, O, O, I, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, R, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, A, R, C, R, O, O, G, R, O, O, R, O, R, R, O,
  O, O, R, R, T, O, R, R, R, R, O, O, O, O, O, O,
  O, A, R, C, R, O, O, G, R, O, O, R, O, R, R, O,
  O, O, R, R, T, O, R, R, R, R, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O,
  O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O
};
#undef R
#undef I
#undef O
#undef A
#undef C
#undef G
#undef T
static const char rev_codes[4] = { 'A', 'C', 'G', 'T' };

class different_k_error : public std::length_error {
public:
  different_k_error() : std::length_error("Different k-mer length") {};
};

class error_short_string : public std::length_error {
public:
  error_short_string() : std::length_error("Input string is too short") {};
};


// Checkered mask. cmask<uint16_t, 1> is every other bit on
// (0x55). cmask<uint16_t,2> is two bits one, two bits off (0x33). Etc.
template<typename U, int len, int l = sizeof(U) * 8 / (2 * len)>
struct cmask {
  static const U v =
    (cmask<U, len, l - 1>::v << (2 * len)) | (((U)1 << len) - 1);
};
template<typename U, int len>
struct cmask<U, len, 0> {
  static const U v = 0;
};

// Fast reverse complement of one word through bit tweedling.
inline uint32_t word_reverse_complement(uint32_t w) {
  typedef uint32_t U;
  w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
  w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
  w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
  w = ( w >> 16                   ) | ( w                    << 16);
  return ((U)-1) - w;
}

inline uint64_t word_reverse_complement(uint64_t w) {
  typedef uint64_t U;
  w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
  w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
  w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
  w = ((w >> 16) & cmask<U, 16>::v) | ((w & cmask<U, 16>::v) << 16);
  w = ( w >> 32                   ) | ( w                    << 32);
  return ((U)-1) - w;
}

#ifdef HAVE_INT128
inline unsigned __int128 word_reverse_complement(unsigned __int128 w) {
  typedef unsigned __int128 U;
  w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
  w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
  w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
  w = ((w >> 16) & cmask<U, 16>::v) | ((w & cmask<U, 16>::v) << 16);
  w = ((w >> 32) & cmask<U, 32>::v) | ((w & cmask<U, 32>::v) << 32);
  w = ( w >> 64                   ) | ( w                    << 64);
  return ((U)-1) - w;
}
#endif

template<typename T>
class base_proxy {
public:
  typedef T base_type;

  base_proxy(base_type* w, unsigned int i) :
    word_(w), i_(i) { }

  base_proxy& operator=(char base) { return this->operator=(codes[(int)(unsigned char)base]); }
  base_proxy& operator=(int code) {
    base_type mask = (base_type)0x3 << i_;
    *word_ = (*word_ & ~mask) | ((base_type)code << i_);
    return *this;
  }
  int code() const { return (*word_ >> i_) & (base_type)0x3; }
  operator char() const { return rev_codes[code()]; }

private:
  base_type* const word_;
  unsigned int     i_;
};

// enum { CODE_A, CODE_C, CODE_G, CODE_T,
//        CODE_RESET = -1, CODE_IGNORE = -2, CODE_COMMENT = -3 };

template<typename T>
struct mer_dna_traits { };

template<typename derived>
class mer_base {
public:
  typedef typename mer_dna_traits<derived>::base_type base_type;

  enum { CODE_A, CODE_C, CODE_G, CODE_T,
         CODE_RESET = -1, CODE_IGNORE = -2, CODE_COMMENT = -3 };

  int get_code(unsigned int i) const {
    if (i >= this->k())
      throw std::out_of_range("Index out of range");

    unsigned int bit_idx = 2 * (this->k() - 1 - i) % (wbits);
    unsigned int word_idx = (this->k() - 1 - i) / (wbases);

    int code = ((data()[word_idx] >> bit_idx) & c3);

    return code;
  }

  char get_nucleotide(unsigned int code) const {
    switch (code) {
      case CODE_A: return 'A';
      case CODE_C: return 'C';
      case CODE_G: return 'G';
      case CODE_T: return 'T';
      default:     return 'N'; 
    }
}

  operator derived() { return *static_cast<derived*>(this); }
  operator const derived() const { return *static_cast<const derived*>(this); }
  unsigned int k() const { return static_cast<const derived*>(this)->k(); }

  /// Direct access to data. No bound or consistency check. Use with
  /// caution!
  //  base_type operator[](unsigned int i) { return data()[i]; }
  base_type word(unsigned int i) const { return data()[i]; }
  base_type operator[](unsigned int i) const { return data()[i]; }
  /// Direct access to the data array.
  const base_type* data() const { return static_cast<const derived*>(this)->data(); }

  /// Same as above, but can modify directly content. Use at your own
  /// risk!
  base_type& word__(unsigned int i) { return data__()[i]; }
  base_type* data__() { return static_cast<derived*>(this)->data__(); }

  template<unsigned int alignment>
  void read(std::istream& is) {
    const unsigned int k = static_cast<const derived*>(this)->k();
    const unsigned int l = k / (4 * alignment) + (k % (4 * alignment) != 0);
    is.read((char*)data__(), l);
  }

  bool operator==(const mer_base& rhs) const {
    auto* const _data = data();
    auto* const rhs_data = rhs.data();
    unsigned int i = nb_words() - 1;
    bool res = (_data[i] & msw()) == (rhs_data[i] & msw());
    while(res && i > 7) {
      i -= 8;
      res = res && (_data[i+7] == rhs_data[i+7]);
      res = res && (_data[i+6] == rhs_data[i+6]);
      res = res && (_data[i+5] == rhs_data[i+5]);
      res = res && (_data[i+4] == rhs_data[i+4]);
      res = res && (_data[i+3] == rhs_data[i+3]);
      res = res && (_data[i+2] == rhs_data[i+2]);
      res = res && (_data[i+1] == rhs_data[i+1]);
      res = res && (_data[i]   == rhs_data[i]  );
    }
    switch(i) {
    case 7: res = res && (_data[6] == rhs_data[6]); [[fallthrough]];
    case 6: res = res && (_data[5] == rhs_data[5]); [[fallthrough]];
    case 5: res = res && (_data[4] == rhs_data[4]); [[fallthrough]];
    case 4: res = res && (_data[3] == rhs_data[3]); [[fallthrough]];
    case 3: res = res && (_data[2] == rhs_data[2]); [[fallthrough]];
    case 2: res = res && (_data[1] == rhs_data[1]); [[fallthrough]];
    case 1: res = res && (_data[0] == rhs_data[0]);
    }
    return res;
  }

  bool operator!=(const mer_base& rhs) const { return !this->operator==(rhs); }
  bool operator<(const mer_base& rhs) const {
    auto* const _data = data();
    auto* const rhs_data = rhs.data();
    unsigned int i = nb_words();
    while(i >= 8) {
      i -= 8;
      if(_data[i+7] != rhs_data[i+7]) return _data[i+7] < rhs_data[i+7];
      if(_data[i+6] != rhs_data[i+6]) return _data[i+6] < rhs_data[i+6];
      if(_data[i+5] != rhs_data[i+5]) return _data[i+5] < rhs_data[i+5];
      if(_data[i+4] != rhs_data[i+4]) return _data[i+4] < rhs_data[i+4];
      if(_data[i+3] != rhs_data[i+3]) return _data[i+3] < rhs_data[i+3];
      if(_data[i+2] != rhs_data[i+2]) return _data[i+2] < rhs_data[i+2];
      if(_data[i+1] != rhs_data[i+1]) return _data[i+1] < rhs_data[i+1];
      if(_data[i]   != rhs_data[i])   return _data[i]   < rhs_data[i];
    }
    switch(i) {
    case 7: if(_data[6] != rhs_data[6]) return _data[6] < rhs_data[6]; [[fallthrough]];
    case 6: if(_data[5] != rhs_data[5]) return _data[5] < rhs_data[5]; [[fallthrough]];
    case 5: if(_data[4] != rhs_data[4]) return _data[4] < rhs_data[4]; [[fallthrough]];
    case 4: if(_data[3] != rhs_data[3]) return _data[3] < rhs_data[3]; [[fallthrough]];
    case 3: if(_data[2] != rhs_data[2]) return _data[2] < rhs_data[2]; [[fallthrough]];
    case 2: if(_data[1] != rhs_data[1]) return _data[1] < rhs_data[1]; [[fallthrough]];
    case 1: if(_data[0] != rhs_data[0]) return _data[0] < rhs_data[0];
    }
    return false;
  }
  bool operator<=(const mer_base& rhs) const {
    return *this < rhs || *this == rhs;
  }
  bool operator>(const mer_base& rhs) const {
    return !(*this <= rhs);
  }
  bool operator>=(const mer_base& rhs) const {
    return !(*this < rhs);
  }

  base_proxy<base_type> base(unsigned int i) { return base_proxy<base_type>(data__() + i / wbases, 2 * (i % wbases)); }
  const base_proxy<base_type> base(unsigned int i) const { return base_proxy<base_type>(const_cast<base_type*>(data()) + i / wbases, 2 * (i % wbases)); }

  // Make current k-mer all As.
  void polyA() { memset(data__(), 0x00, sizeof(base_type) * nb_words()); clean_msw(); }
  void polyC() { memset(data__(), 0x55, sizeof(base_type) * nb_words()); clean_msw(); }
  void polyG() { memset(data__(), 0xaa, sizeof(base_type) * nb_words()); clean_msw(); }
  void polyT() { memset(data__(), 0xff, sizeof(base_type) * nb_words()); clean_msw(); }
  template<typename EngineT>
  void randomize(EngineT& prg) {
    auto* const _data = data__();
    std::uniform_int_distribution<base_type> bits;
    for(unsigned int i = 0; i < nb_words(); ++i)
      _data[i] = bits(prg);
    clean_msw();
  }

  inline void randomize() {
    static thread_local std::default_random_engine eng;
    return randomize(eng);
  }

  bool is_homopolymer() const {
    auto* const _data = data();
    const base_type    base    = _data[0] & c3;
    const unsigned int barrier = nb_words();
    unsigned int       i       = 0;

    for( ; i + 5 < barrier; i += 4) {
      if(_data[i    ] != ((_data[i    ] << 2) | base)) return false;
      if(_data[i + 1] != ((_data[i + 1] << 2) | base)) return false;
      if(_data[i + 2] != ((_data[i + 2] << 2) | base)) return false;
      if(_data[i + 3] != ((_data[i + 3] << 2) | base)) return false;
    }

    switch(nb_words() - i) {
    case 5: if(_data[i] !=  ((_data[i] << 2) | base)         ) return false; ++i; [[fallthrough]];
    case 4: if(_data[i] !=  ((_data[i] << 2) | base)         ) return false; ++i; [[fallthrough]];
    case 3: if(_data[i] !=  ((_data[i] << 2) | base)         ) return false; ++i; [[fallthrough]];
    case 2: if(_data[i] !=  ((_data[i] << 2) | base)         ) return false; ++i; [[fallthrough]];
    case 1: if(_data[i] != (((_data[i] << 2) | base) & msw())) return false;
    }

    return true;
  }

  derived& operator=(const mer_base& rhs) {
    if(this != &rhs)
      memcpy(data__(), rhs.data(), nb_words() * sizeof(base_type));
    return *static_cast<derived*>(this);
  }

  derived& operator=(const char* s) {
    if(strlen(s) < static_cast<derived*>(this)->k())
      throw error_short_string();
    from_chars(s);
    return *static_cast<derived*>(this);
  }

  derived& operator=(const std::string& s) {
    if(s.size() < static_cast<derived*>(this)->k())
      throw error_short_string();
    from_chars(s.c_str());
    return *static_cast<derived*>(this);
  }

  // Shift the k-mer by 1 base, left or right. The char version take
  // a base 'A', 'C', 'G', or 'T'. The base_type version takes a code
  // in [0, 3] (not check of validity of argument, taken modulo
  // 4). The return value is the base that was pushed off the side
  // ('N' if the input character is not a valid base).
  base_type shift_left(int c) {
    auto* const _data = data__();
    const base_type    r       = (_data[nb_words()-1] >> lshift()) & c3;
    const unsigned int barrier = nb_words() & (~c3);
    base_type          c2;    // c2 and c1: carries
    base_type          c1      = (base_type)c & c3;
    unsigned int       i       = 0;

    for( ; i < barrier; i += 4) {
      c2 = _data[i]   >> wshift;   _data[i]   = (_data[i]   << 2) | c1;
      c1 = _data[i+1] >> wshift;   _data[i+1] = (_data[i+1] << 2) | c2;
      c2 = _data[i+2] >> wshift;   _data[i+2] = (_data[i+2] << 2) | c1;
      c1 = _data[i+3] >> wshift;   _data[i+3] = (_data[i+3] << 2) | c2;
    }
    c2 = c1;

    switch(nb_words() - i) {
    case 3: c2 = _data[i] >> wshift;   _data[i] = (_data[i] << 2) | c1;   ++i; [[fallthrough]];
    case 2: c1 = _data[i] >> wshift;   _data[i] = (_data[i] << 2) | c2;   ++i; [[fallthrough]];
    case 1:                            _data[i] = (_data[i] << 2) | c1;
    }
    clean_msw();

    return r;
  }

  base_type shift_right(int c) {
    auto* const _data = data__();
    const base_type r = _data[0] & c3;
    if(nb_words() > 1){
      const unsigned int barrier = (nb_words() - 1) & (~c3);
      unsigned int i = 0;

      for( ; i < barrier; i += 4) {
        _data[i]   = (_data[i]   >> 2) | (_data[i+1] << wshift);
        _data[i+1] = (_data[i+1] >> 2) | (_data[i+2] << wshift);
        _data[i+2] = (_data[i+2] >> 2) | (_data[i+3] << wshift);
        _data[i+3] = (_data[i+3] >> 2) | (_data[i+4] << wshift);
      }
      switch(nb_words() - 1 - i) {
      case 3: _data[i] = (_data[i] >> 2) | (_data[i+1] << wshift);  ++i; [[fallthrough]];
      case 2: _data[i] = (_data[i] >> 2) | (_data[i+1] << wshift);  ++i; [[fallthrough]];
      case 1: _data[i] = (_data[i] >> 2) | (_data[i+1] << wshift);
      }
    }

    _data[nb_words() - 1] =
      ((_data[nb_words() - 1] & msw()) >> 2) | (((base_type)c & c3) << lshift());

    return r;
  }

  // Non DNA codes are negative
  inline static bool not_dna(int c) { return c < 0; }
  inline static int code(char c) { return codes[(int)(unsigned char)c]; }
  inline static char rev_code(int x) { return rev_codes[x]; }
  static int complement(int x) { return (base_type)3 - x; }
  static char complement(char c) {
    switch(c) {
    case 'A': case 'a': return 'T';
    case 'C': case 'c': return 'G';
    case 'G': case 'g': return 'C';
    case 'T': case 't': return 'A';
    }
    return 'N';
  }

  char shift_left(char c) {
    int x = code(c);
    if(x == -1)
      return 'N';
    return rev_code(shift_left(x));
  }

  char shift_right(char c) {
    int x = code(c);
    if(x == -1)
      return 'N';
    return rev_code(shift_right(x));
  }

  void reverse_complement() {
    auto* const _data = data__();
    base_type *low  = _data;
    base_type *high = _data + nb_words() - 1;
    for( ; low < high; ++low, --high) {
      base_type tmp = word_reverse_complement(*low);
      *low          = word_reverse_complement(*high);
      *high         = tmp;
    }
    if(low == high)
      *low = word_reverse_complement(*low);
    unsigned int rs = wbits - nb_msb();
    if(rs > 0)
      large_shift_right(rs);
  }

  void canonicalize() {
    derived rc = this->get_reverse_complement();
    if(rc < *this)
      *this = rc;
  }

  derived get_reverse_complement() const {
    derived res(*this);
    res.reverse_complement();
    return res;
  }

  derived get_canonical() const {
    derived rc = this->get_reverse_complement();
    return rc < *this ? rc : *this;
  }

  // Transfomr the k-mer into a C++ string.
  std::string to_str() const {
    std::string res(static_cast<const derived*>(this)->k(), '\0');
    to_chars(res.begin());
    return res;
  }

  // Transform the k-mer into a string. For the char * version,
  // assume that the buffer is large enough to receive k+1
  // characters (space for '\0' at end of string).
  void to_str(char* s) const {
    s = to_chars(s);
    *s = '\0';
  }

  // Copy bases as char to the output iterator it. No '\0' is added
  // or check made that there is enough space. The iterator pointed
  // after the last base is returned.
  template<typename OutputIterator>
  OutputIterator to_chars(OutputIterator it) const {
    auto* const _data = data();
    int shift  = lshift(); // Number of bits to shift to get base

    for(int j = nb_words() - 1; j >= 0; --j) {
      base_type w = _data[j];
      for( ; shift >= 0; shift -= 2, ++it)
        *it = rev_code((w >> shift) & c3);
      shift = wshift;
    }
    return it;
  }

  // Get bits [start, start+len). start must be < 2k, len <=
  // sizeof(base_type) and start+len < 2k. No checks
  // performed. start and len are in bits, not bases.
  base_type get_bits(unsigned int start, unsigned int len) const {
    auto* const _data = data();
    unsigned int q = start / wbits;
    unsigned int r = start % wbits;

    base_type res = _data[q] >> r;
    if(len > wbits - r)
      res |= _data[q + 1] << (wbits - r);
    return len < (unsigned int)wbits ? res & (((base_type)1 << len) - 1) : res;
  }

  // Set bits [start, start+len). Same restriction as get_bits. In
  // some rare cases, the value written can be larger than the bits
  // occupied by the mer itself. The mer is then not valid if some MSB
  // are set to 1.
  template<bool zero_msw = true>
  void set_bits(unsigned int start, unsigned int len, base_type v) {
    auto* const _data = data__();
    unsigned int q    = start / wbits;
    unsigned int r    = start % wbits;
    unsigned int left = wbits - r;
    base_type    mask;
    if(len > left) {
      mask       = ((base_type)1 << r) - 1;
      _data[q]   = (_data[q] & mask) | (v << r);
      mask = ((base_type)1 << (len - left)) - 1;
      _data[q + 1] = (_data[q + 1] & ~mask) | (v >> (left));
    } else {
      mask = (len < (unsigned int)wbits ? (((base_type)1 << len) - 1) : (base_type)-1) << r;
      _data[q] = (_data[q] & ~mask) | (v << r);
    }
    if(zero_msw)
      clean_msw();
  }



  // Internal stuff

  // Number of words in the data array
  inline static unsigned int nb_words(unsigned int k) { return (k / wbases) + (k % wbases != 0); }
  inline unsigned int nb_words() const {
    // Return a validated count so inline-array bounds are visible to the compiler.
    return derived::check_size(nb_words(static_cast<const derived*>(this)->k()));
  }
  inline size_t nb_bytes() const { return nb_words() * sizeof(base_type); }

  // Mask of highest word
  inline base_type msw() const {
    const base_type m = std::numeric_limits<base_type>::max();
    return m >> (wbits - nb_msb());
  }

  // Nb of bits used in highest word
  inline  unsigned int nb_msb() const {
    base_type nb = (static_cast<const derived*>(this)->k() % wbases) * 2;
    return nb ? nb : wbits;
  }
  // How much to shift last base in the last data word
  inline unsigned int lshift() const { return nb_msb() - 2; }

  // Make sure the highest bits are all zero
  inline void clean_msw() { data__()[nb_words() - 1] &= msw(); }

  // Without an end iterator, the caller must provide at least k characters.
  template<typename InputIterator>
  bool from_chars(InputIterator it) {
    return from_chars_impl<false>(it, it);
  }

  template<typename InputIterator>
  bool from_chars(InputIterator it, InputIterator end) {
    return from_chars_impl<true>(it, end);
  }

protected:
  static const base_type c3     = (base_type)0x3;
  static const int       wshift = sizeof(base_type) * 8 - 2; // left shift in 1 word
  static const int       wbases = 4 * sizeof(base_type); // bases in a word
  static const int       wbits  = 8 * sizeof(base_type); // bits in a word

  template<bool Bounded, typename InputIterator>
  bool from_chars_impl(InputIterator it, InputIterator end) {
    auto* const _data = data__();
    int shift = lshift();
    clean_msw();

    for(int j = nb_words() - 1; j >= 0; --j) {
      base_type& w = _data[j];
      w = 0;
      for( ; shift >= 0; shift -= 2, ++it) {
        if(Bounded && it == end)
          return false;
        int c = code(*it);
        if(not_dna(c))
          return false;
        w |= (base_type)c << shift;
      }
      shift = wshift;
    }
    return true;
  }

  template<typename U>
  void assign_words(const U& rhs) {
    auto* const _data = data__();
    for(unsigned int i = 0; i < nb_words(); ++i)
      _data[i] = rhs[i];
    clean_msw();
  }

  // Shift to the right by rs bits (Note bits, not bases)
  void large_shift_right(unsigned int rs) {
    auto* const _data = data__();
    if(nb_words() > 1) {
      const unsigned int barrier = (nb_words() - 1) & (~c3);
      const unsigned int ls = wbits - rs;
      unsigned int i = 0;

      for( ; i < barrier; i += 4) {
        _data[i]   = (_data[i]   >> rs) | (_data[i+1] << ls);
        _data[i+1] = (_data[i+1] >> rs) | (_data[i+2] << ls);
        _data[i+2] = (_data[i+2] >> rs) | (_data[i+3] << ls);
        _data[i+3] = (_data[i+3] >> rs) | (_data[i+4] << ls);
      }
      switch(nb_words() - 1 - i) {
      case 3: _data[i] = (_data[i] >> rs) | (_data[i+1] << ls); ++i; [[fallthrough]];
      case 2: _data[i] = (_data[i] >> rs) | (_data[i+1] << ls); ++i; [[fallthrough]];
      case 1: _data[i] = (_data[i] >> rs) | (_data[i+1] << ls);
      }
    }
    _data[nb_words() - 1] >>= rs;
    clean_msw();
  }
};

// Storage policies receive a word count, independent of the k-mer length.
// The length wrappers handle value copying after their storage is initialized.
// check_size returns the validated count, preserving its bounds for algorithms.
template<typename T = uint64_t>
class mer_base_heap {
public:
  typedef T base_type;

  explicit mer_base_heap(unsigned int words) : data_(new base_type[words]()) { }
  ~mer_base_heap() { delete[] data_; }

  mer_base_heap(const mer_base_heap&) = delete;
  mer_base_heap& operator=(const mer_base_heap&) = delete;

  static unsigned int check_size(unsigned int words) { return words; }
  const base_type* data() const { return data_; }
  base_type* data__() { return data_; }

private:
  base_type* data_;
};

template<typename T, unsigned int W>
class mer_base_stack {
  static_assert(W > 0, "Stack storage must contain at least one word");

public:
  typedef T base_type;

  explicit mer_base_stack(unsigned int words) : data_{} { check_size(words); }

  static unsigned int check_size(unsigned int words) {
    if(words == 0 || words > W)
      throw std::length_error("K-mer length is outside stack storage capacity");
    return words;
  }
  const base_type* data() const { return data_; }
  base_type* data__() { return data_; }

private:
  base_type data_[W];
};

// Mer type where the length is kept in each mer object: allows to
// manipulate mers of different size within the same application.
template<typename T = uint64_t, typename Storage = mer_base_heap<T>>
class mer_base_dynamic : public Storage, public mer_base<mer_base_dynamic<T, Storage>> {
  static_assert(std::is_same<T, typename Storage::base_type>::value,
                "Storage word type must match the k-mer word type");

public:
  typedef T base_type;
  typedef mer_base<mer_base_dynamic<T, Storage>> super;

  explicit mer_base_dynamic(unsigned int k) : Storage(super::nb_words(k)), k_(k) { }
  mer_base_dynamic(const mer_base_dynamic& rhs) : mer_base_dynamic(rhs.k()) {
    super::operator=(rhs);
  }
  mer_base_dynamic(unsigned int k, const char* s) : mer_base_dynamic(k) {
    super::from_chars(s);
  }
  explicit mer_base_dynamic(const char* s) : mer_base_dynamic(strlen(s)) {
    super::from_chars(s);
  }
  explicit mer_base_dynamic(const std::string& s) : mer_base_dynamic(s.size()) {
    super::from_chars(s.begin());
  }

  template<typename U>
  explicit mer_base_dynamic(unsigned int k, const U& rhs) : mer_base_dynamic(k) {
    super::assign_words(rhs);
  }

  mer_base_dynamic& operator=(const mer_base_dynamic& rhs) {
    if(k_ != rhs.k_)
      throw different_k_error();
    super::operator=(rhs);
    return *this;
  }

  const base_type* data() const { return Storage::data(); }
  base_type* data__() { return Storage::data__(); }

  unsigned int k() const { return k_; }
  static unsigned int k(unsigned int k) {
    Storage::check_size(super::nb_words(k));
    return k;
  }

private:
  const unsigned int k_;
};

template<typename T, typename Storage>
struct mer_dna_traits<mer_base_dynamic<T, Storage>> {
  typedef T base_type;
};

// Mer type where the length is a static variable: the mer size is
// fixed for all k-mers in the application.
//
// The CI (Class Index) template parameter allows to have more than one such
// class with different length in the same application. Each complete template
// specialization, including its storage policy, has its own static length.
template<typename T = uint64_t, int CI = 0, typename Storage = mer_base_heap<T>>
class mer_base_static : public Storage, public mer_base<mer_base_static<T, CI, Storage>> {
  static_assert(std::is_same<T, typename Storage::base_type>::value,
                "Storage word type must match the k-mer word type");

public:
  typedef T base_type;
  typedef mer_base<mer_base_static<T, CI, Storage>> super;
  static const int class_index = CI;

  mer_base_static() : Storage(super::nb_words(k_)) { }
  explicit mer_base_static(unsigned int k) : mer_base_static() {
    if(k != k_)
      throw different_k_error();
  }
  mer_base_static(const mer_base_static& rhs) : mer_base_static() {
    super::operator=(rhs);
  }

  mer_base_static(unsigned int k, const char* s) : mer_base_static(k) {
    super::from_chars(s);
  }
  explicit mer_base_static(const char* s) : mer_base_static() {
    super::from_chars(s);
  }
  explicit mer_base_static(const std::string& s) : mer_base_static() {
    super::from_chars(s.begin());
  }

  template<typename U>
  mer_base_static(unsigned int k, const U& rhs) : mer_base_static(k) {
    super::assign_words(rhs);
  }

  mer_base_static& operator=(const char* s) { return super::operator=(s); }
  mer_base_static& operator=(const std::string& s) { return super::operator=(s); }
  mer_base_static& operator=(const mer_base_static& rhs) {
    if(k_ != rhs.k_)
      throw different_k_error();
    super::operator=(rhs);
    return *this;
  }

  const base_type* data() const { return Storage::data(); }
  base_type* data__() { return Storage::data__(); }

  static unsigned int k();
  static unsigned int k(unsigned int k) {
    Storage::check_size(super::nb_words(k));
    std::swap(k, k_);
    return k;
  }

private:
  static unsigned int k_;
};
template<typename T, int CI, typename Storage>
unsigned int mer_base_static<T, CI, Storage>::k_ = 22;
template<typename T, int CI, typename Storage>
unsigned int mer_base_static<T, CI, Storage>::k() { return k_; }
template<typename T, int CI, typename Storage>
const int mer_base_static<T, CI, Storage>::class_index;

template<typename T, int CI, typename Storage>
struct mer_dna_traits<mer_base_static<T, CI, Storage>> {
  typedef T base_type;
};

typedef std::ostream_iterator<char> ostream_char_iterator;
template<typename derived>
inline std::ostream& operator<<(std::ostream& os, const mer_base<derived>& mer) {
  std::ostream_iterator<char> oos(os);
  mer.to_chars(oos);
  return os;
}

typedef std::istream_iterator<char> istream_char_iterator;
template<typename derived>
inline std::istream& operator>>(std::istream& is, mer_base<derived>& mer) {
  std::istream::sentry sentry(is);
  if(!sentry)
    return is;

  // Buffer iterators consume exactly k bases, leaving the next field untouched.
  std::istreambuf_iterator<char> current(is), end;
  if(!mer.from_chars(current, end)) {
    std::ios::iostate state = std::ios::failbit;
    if(current == end)
      state |= std::ios::eofbit;
    is.setstate(state);
  }
  return is;
}

} // namespace mer_dna_ns


typedef mer_dna_ns::mer_base_static<uint32_t, 0> mer_dna32;
typedef mer_dna_ns::mer_base_static<uint64_t, 0> mer_dna64;
#ifdef HAVE_INT128
typedef mer_dna_ns::mer_base_static<unsigned __int128, 0> mer_dna128;
#endif

typedef mer_dna64 mer_dna;

} // namespace jellyfish
