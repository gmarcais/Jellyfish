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

#ifndef __JELLYFISH_MER_DNA_HPP__
#define __JELLYFISH_MER_DNA_HPP__

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

#include <jellyfish/misc.hpp>
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


extern const char* const error_different_k;
extern const char* const error_short_string;


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
  typedef uint64_t U;
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

  mer_base() { }

  ~mer_base() { }

  operator derived() { return *static_cast<derived*>(this); }
  operator const derived() const { return *static_cast<const derived*>(this); }
  unsigned int k() const { return static_cast<const derived*>(this)->k(); }

  /// Direct access to data. No bound or consistency check. Use with
  /// caution!
  /// Direct access to the data array.
  const base_type* data() const { return static_cast<const derived*>(this)->data(); }
  base_type word(unsigned int i) const { return data()[i]; }
  base_type operator[](unsigned int i) const { return data()[i]; }

  /// Same as above, but can modify directly content. Use at your own
  /// risk!
  base_type* data__() { return static_cast<derived*>(this)->data(); }
  base_type& word__(unsigned int i) { return data__()[i]; }


  template<unsigned int alignment>
  void read(std::istream& is) {
    const unsigned int k_ = k();
    const unsigned int l = k_ / (4 * alignment) + (k_ % (4 * alignment) != 0);
    is.read((char*)data(), l);
    clean_msw();
  }

  template<typename D>
  bool operator==(const mer_base<D>& rhs) const {
    if(k() != rhs.k())
      return false;

    unsigned int           i         = nb_words() - 1;
    const base_type* const _data     = data();
    const base_type* const _rhs_data = rhs.data();
    bool                   res       = (_data[i] & msw()) == (_rhs_data[i] & msw());

    while(res && i > 7) {
      i -= 8;
      res = res && (_data[i+7] == _rhs_data[i+7]);
      res = res && (_data[i+6] == _rhs_data[i+6]);
      res = res && (_data[i+5] == _rhs_data[i+5]);
      res = res && (_data[i+4] == _rhs_data[i+4]);
      res = res && (_data[i+3] == _rhs_data[i+3]);
      res = res && (_data[i+2] == _rhs_data[i+2]);
      res = res && (_data[i+1] == _rhs_data[i+1]);
      res = res && (_data[i]   == _rhs_data[i]  );
    }
    switch(i) {
    case 7: res = res && (_data[6] == _rhs_data[6]);
    case 6: res = res && (_data[5] == _rhs_data[5]);
    case 5: res = res && (_data[4] == _rhs_data[4]);
    case 4: res = res && (_data[3] == _rhs_data[3]);
    case 3: res = res && (_data[2] == _rhs_data[2]);
    case 2: res = res && (_data[1] == _rhs_data[1]);
    case 1: res = res && (_data[0] == _rhs_data[0]);
    }
    return res;
  }

  template<typename D>
  bool operator!=(const mer_base<D>& rhs) const { return !this->operator==(rhs); }
  template<typename D>
  bool operator<(const mer_base<D>& rhs) const {
    if(k() != rhs.k())
      return k() < rhs.k();
    unsigned int           i         = nb_words();
    const base_type* const _data     = data();
    const base_type* const _rhs_data = rhs.data();

    while(i >= 8) {
      i -= 8;
      if(_data[i+7] != _rhs_data[i+7]) return _data[i+7] < _rhs_data[i+7];
      if(_data[i+6] != _rhs_data[i+6]) return _data[i+6] < _rhs_data[i+6];
      if(_data[i+5] != _rhs_data[i+5]) return _data[i+5] < _rhs_data[i+5];
      if(_data[i+4] != _rhs_data[i+4]) return _data[i+4] < _rhs_data[i+4];
      if(_data[i+3] != _rhs_data[i+3]) return _data[i+3] < _rhs_data[i+3];
      if(_data[i+2] != _rhs_data[i+2]) return _data[i+2] < _rhs_data[i+2];
      if(_data[i+1] != _rhs_data[i+1]) return _data[i+1] < _rhs_data[i+1];
      if(_data[i]   != _rhs_data[i])   return _data[i]   < _rhs_data[i];
    }
    switch(i) {
    case 7: if(_data[6] != _rhs_data[6]) return _data[6] < _rhs_data[6];
    case 6: if(_data[5] != _rhs_data[5]) return _data[5] < _rhs_data[5];
    case 5: if(_data[4] != _rhs_data[4]) return _data[4] < _rhs_data[4];
    case 4: if(_data[3] != _rhs_data[3]) return _data[3] < _rhs_data[3];
    case 3: if(_data[2] != _rhs_data[2]) return _data[2] < _rhs_data[2];
    case 2: if(_data[1] != _rhs_data[1]) return _data[1] < _rhs_data[1];
    case 1: if(_data[0] != _rhs_data[0]) return _data[0] < _rhs_data[0];
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

  // Make current k-mer all As.
  void polyA() { memset(data__(), 0x00, sizeof(base_type) * nb_words()); clean_msw(); }
  void polyC() { memset(data__(), 0x55, sizeof(base_type) * nb_words()); clean_msw(); }
  void polyG() { memset(data__(), 0xaa, sizeof(base_type) * nb_words()); clean_msw(); }
  void polyT() { memset(data__(), 0xff, sizeof(base_type) * nb_words()); clean_msw(); }
  void randomize() {
    base_type* const _data = data__();
    for(unsigned int i = 0; i < nb_words(); ++i)
      _data[i] = random_bits(wbits);
    clean_msw();
  }

  derived& operator=(const mer_base& rhs) {
    memcpy(data__(), rhs.data(), nb_words() * sizeof(base_type));
    return *static_cast<derived*>(this);
  }

  template<typename D>
  derived& operator=(const mer_base<D>& rhs) {
    if(k() != rhs.k())
      throw std::length_error(error_different_k);
    memcpy(data__(), rhs.data(), nb_words() * sizeof(base_type));
    return *static_cast<derived*>(this);
  }

  derived& operator=(const char* s) {
    if(strlen(s) < k())
      throw std::length_error(error_short_string);
    from_chars(s);
    return *static_cast<derived*>(this);
  }

  derived& operator=(const std::string& s) {
    if(s.size() < static_cast<derived*>(this)->k())
      throw std::length_error(error_short_string);
    from_chars(s.c_str());
    return *static_cast<derived*>(this);
  }

  // Shift the k-mer by 1 base, left or right. The char version take
  // a base 'A', 'C', 'G', or 'T'. The base_type version takes a code
  // in [0, 3] (not check of validity of argument, taken modulo
  // 4). The return value is the base that was pushed off the side
  // ('N' if the input character is not a valid base).
  base_type shift_left(int c) {
    base_type* const   _data   = data__();
    const base_type    r       = (_data[nb_words()-1] >> lshift()) & c3;
    const unsigned int barrier = nb_words() & (~c3);
    base_type          c2;      // c2 and c1: carries
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
    case 3: c2 = _data[i] >> wshift;   _data[i] = (_data[i] << 2) | c1;   ++i;
    case 2: c1 = _data[i] >> wshift;   _data[i] = (_data[i] << 2) | c2;   ++i;
    case 1:                            _data[i] = (_data[i] << 2) | c1;
    }
    clean_msw();

    return r;
  }

  base_type shift_right(int c) {
    base_type* const _data = data__();
    const base_type  r     = _data[0] & c3;
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
      case 3: _data[i] = (_data[i] >> 2) | (_data[i+1] << wshift);  ++i;
      case 2: _data[i] = (_data[i] >> 2) | (_data[i+1] << wshift);  ++i;
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
    base_type* low  = data__();
    base_type* high = low + nb_words() - 1;
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
    int shift  = lshift(); // Number of bits to shift to get base

    for(int j = nb_words() - 1; j >= 0; --j) {
      base_type w = word(j);
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
    unsigned int q = start / wbits;
    unsigned int r = start % wbits;

    base_type res = word(q) >> r;
    if(len > wbits - r)
      res |= word(q + 1) << (wbits - r);
    return len < (unsigned int)wbits ? res & (((base_type)1 << len) - 1) : res;
  }

  // Set bits [start, start+len). Same restriction as get_bits. In
  // some rare cases, the value written can be larger than the bits
  // occupied by the mer itself. The mer is then not valid if some MSB
  // are set to 1.
  template<bool zero_msw = true>
  void set_bits(unsigned int start, unsigned int len, base_type v) {
    base_type* const _data = data__();
    unsigned int     q     = start / wbits;
    unsigned int     r     = start % wbits;
    unsigned int     left  = wbits - r;
    base_type        mask;
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

  // Number of words in _data
  inline static unsigned int nb_words(unsigned int kk) { return (kk / wbases) + (kk % wbases != 0); }
  inline unsigned int nb_words() const { return nb_words(k()); }

  // Mask of highest word
  inline base_type msw() const {
    const base_type m = std::numeric_limits<base_type>::max();
    return m >> (wbits - nb_msb());
  }

  // Nb of bits used in highest word
  inline  unsigned int nb_msb() const {
    base_type nb = (k() % wbases) * 2;
    return nb ? nb : wbits;
  }
  // How much to shift last base in last word of _data
  inline unsigned int lshift() const { return nb_msb() - 2; }

  // Make sure the highest bits are all zero
  inline void clean_msw() { data__()[nb_words() - 1] &= msw(); }

  template<typename InputIterator>
  bool from_chars(InputIterator it) {
    base_type* _data = data__();
    int shift = lshift();
    clean_msw();

    for(int j = nb_words() - 1; j >= 0; --j) {
      base_type& w = _data[j];
      w = 0;
      for( ; shift >= 0; shift -= 2, ++it) {
        int c = code(*it);
        if(not_dna(c))
          return false;
        w |= (base_type)c << shift;
      }
      shift = wshift;
    }
    return true;
  }

protected:
  static const base_type c3     = (base_type)0x3;
  static const int       wshift = sizeof(base_type) * 8 - 2; // left shift in 1 word
  static const int       wbases = 4 * sizeof(base_type); // bases in a word
  static const int       wbits  = 8 * sizeof(base_type); // bits in a word

  // Shift to the right by rs bits (Note bits, not bases)
  void large_shift_right(unsigned int rs) {
    base_type* const _data = data__();
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
      case 3: _data[i] = (_data[i] >> rs) | (_data[i+1] << ls); ++i;
      case 2: _data[i] = (_data[i] >> rs) | (_data[i+1] << ls); ++i;
      case 1: _data[i] = (_data[i] >> rs) | (_data[i+1] << ls);
      }
    }
    _data[nb_words() - 1] >>= rs;
    clean_msw();
  }
};

template<typename T>
struct mer_dna_new {
  T* data_;
  mer_dna_new(size_t words) : data_(new T[words]) { }
  mer_dna_new(mer_dna_new&& rhs) : data_(rhs.data_) { rhs.data_ = 0; }
  mer_dna_new& operator=(mer_dna_new&& rhs) {
    std::swap(data_, rhs.data_);
    return *this;
  }
  ~mer_dna_new() { delete [] data_; }
};

template<typename T>
struct mer_dna_exist {
  T* data_;
  mer_dna_exist(T* data) : data_(data) { }
  mer_dna_exist(mer_dna_exist&& rhs) : data_(rhs.data_) { }
  mer_dna_exist& operator=(mer_dna_exist&& rhs) {
    data_ = rhs.data_;
    return *this;
  }
  ~mer_dna_exist() { }
};

// Mer type where the length is kept in each mer object: allows to
// manipulate mers of different size within the same application.
template<typename T = uint64_t, typename Memory = struct mer_dna_new<T> >
class mer_base_dynamic : public mer_base<mer_base_dynamic<T> > {
public:
  typedef T base_type;
  typedef mer_base<mer_base_dynamic<T> > super;

  explicit mer_base_dynamic(unsigned int kk) : k_(kk), mem_(super::nb_words(k_)) {
    super::clean_msw();
  }
  mer_base_dynamic(unsigned int kk, Memory&& m) : k_(kk), mem_(std::move(m)) {
    super::clean_msw();
  }
  template<typename D>
  mer_base_dynamic(const mer_base<D>& rhs) : k_(rhs.k()), mem_(super::nb_words(k_)) {
    *this = rhs;
  }
  mer_base_dynamic(mer_base_dynamic&& rhs) : k_(rhs.k()), mem_(std::move(rhs.mem_)) { }
  mer_base_dynamic(const mer_base_dynamic& rhs) : k_(rhs.k()), mem_(super::nb_words(rhs.k())) {
    *this = rhs;
  }
  mer_base_dynamic(const mer_base_dynamic& rhs, Memory&& m) : k_(rhs.k()), mem_(std::move(m)) {
    *this = rhs;
  }
  mer_base_dynamic(unsigned int kk, const char* s) : k_(kk), mem_(super::nb_words(k_)) {
    super::from_chars(s);
  }
  mer_base_dynamic(unsigned int kk, const char* s, Memory&& m) : k_(kk), mem_(std::move(m)) {
    super::from_chars(s);
  }
  explicit mer_base_dynamic(const char* s) : k_(strlen(s)), mem_(super::nb_words(k_)) {
    super::from_chars(s);
  }
  mer_base_dynamic(const char* s, Memory&& m) : k_(strlen(s)), mem_(std::move(m)) {
    super::from_chars(s);
  }

  explicit mer_base_dynamic(const std::string& s) : k_(s.size()), mem_(super::nb_words(k_)) {
    super::from_chars(s.begin());
  }
  mer_base_dynamic(const std::string& s, Memory&& m) : k_(s.size()), mem_(std::move(m)) {
    super::from_chars(s.begin());
  }

  // template<typename U>
  // explicit mer_base_dynamic(unsigned int k, const U& rhs) : super(k, rhs), k_(k) { }

  ~mer_base_dynamic() { }

  mer_base_dynamic& operator=(const mer_base_dynamic& rhs) {
    if(k_ != rhs.k_)
      throw std::length_error(error_different_k);
    super::operator=(rhs);
    return *this;
  }

  mer_base_dynamic& operator=(mer_base_dynamic&& rhs) {
    if(k_ != rhs.k_)
      throw std::length_error(error_different_k);
    mem_ = std::move(rhs.mem_);
    return *this;
  }

  unsigned int k() const { return k_; }
  static unsigned int k(unsigned int k) { return k; }
  base_type* data() { return mem_.data_; }
  const base_type* data() const { return mem_.data_; }

private:
  const unsigned int k_;
  Memory mem_;
};

template<typename T>
struct mer_dna_traits<mer_base_dynamic<T> > {
  typedef T base_type;
};

// Mer type where the length is a static variable: the mer size is
// fixed for all k-mers in the application.
//
// The CI (Class Index) template parameter allows to have more than one such
                                                                                                               // class with different length in the same application. Each class has
// its own static variable associated with it.
template<int CI>
struct mer_len_static {
  static unsigned int k_;
  static constexpr int class_index() { return CI; }
};
template<int CI>
unsigned int mer_len_static<CI>::k_ = 22;

template<typename T = uint64_t, typename MerLen = struct mer_len_static<0>, typename Memory = struct mer_dna_new<T> >
class mer_base_static : public mer_base<mer_base_static<T, MerLen, Memory> > {
public:
  typedef T base_type;
  typedef mer_base<mer_base_static<T, MerLen, Memory> > super;

  mer_base_static(Memory&& m = Memory(super::nb_words(MerLen::k_))) : mem_(std::move(m)) { super::clean_msw(); }
  mer_base_static(const mer_base_static& rhs) : mem_(super::nb_words(MerLen::k_)) {
    *this = rhs;
  }
  mer_base_static(mer_base_static&& rhs) : mem_(std::move(rhs.mem_)) { }
  explicit mer_base_static(unsigned int kk) : mem_(super::nb_words(MerLen::k_)) {
    if(kk != MerLen::k_)
      throw std::length_error(error_different_k);
  }
  explicit mer_base_static(unsigned int kk, Memory&& m) : mem_(std::move(m)) {
    if(kk != MerLen::k_)
      throw std::length_error(error_different_k);
  }
  template<typename D>
  mer_base_static(const mer_base<D>& rhs) : mem_(super::nb_words(rhs.k())) {
    *this = rhs;
  }
  template<typename D>
  mer_base_static(const mer_base<D>& rhs, Memory&& m) : mem_(std::move(m)) {
    *this = rhs;
  }

  mer_base_static(unsigned int k, const char* s, Memory&& m = Memory(super::nb_words(MerLen::k_))) : mem_(std::move(m)) {
    super::from_chars(s);
  }
  explicit mer_base_static(const char* s, Memory&& m = Memory(super::nb_words(MerLen::k_))) : mem_(std::move(m)) {
    super::from_chars(s);
  }
  explicit mer_base_static(const std::string& s, Memory&& m = Memory(super::nb_words(MerLen::k_))) : mem_(std::move(m)) {
    super::from_chars(s.begin());
  }

  // template<typename U>
  // mer_base_static(unsigned int k, const U& rhs) : super(MerLen::k_, rhs) {
  //   if(k != MerLen::k_)
  //     throw std::length_error(error_different_k);
  // }

  mer_base_static& operator=(const char* s) {
    super::operator=(s);
    return *this;
  }
  mer_base_static& operator=(const std::string& s) {
    super::operator=(s);
    return *this;
  }
  mer_base_static& operator=(const mer_base_static& rhs) { return super::operator=(rhs); }
  template<typename D>
  mer_base_static& operator=(const mer_base<D>& rhs) {
    super::operator=(rhs);
    return *this;
  }

  ~mer_base_static() { }

  static unsigned int k() { return MerLen::k_; }
  static unsigned int k(unsigned int k) {
    unsigned int old = MerLen::k_;
    MerLen::k_ = k;
    return old;
  }

  static constexpr int class_index() { return MerLen::class_index(); }
  base_type* data() { return mem_.data_; }
  const base_type* data() const { return mem_.data_; }

  static size_t nb_words() { return super::nb_words(k()); }

private:
  Memory mem_;
};

template<typename T, typename ML, typename M>
struct mer_dna_traits<mer_base_static<T, ML, M> > {
  typedef T base_type;
  typedef ML mer_len_type;
};

// template<typename T = uint64_t, int CI = 0>
// class mer_dna_stack : public mer_base<mer_dna_stack<T, CI> > {
//   typedef mer_base<mer_dna_stack<T, CI> > super;
//   typedef mer_dna_exist<T> mem_type;
// public:
//   mer_dna_stack(T* a) : super(mem_type(a)) { }
//   mer_dna_stack(unsigned int k, T* a) : super(k, mem_type(a)) { }
//   template<typename D>
//   mer_dna_stack(const mer_base<D>& rhs, T* a) : super(rhs, mem_type(a)) { }

//   mer_dna_stack& operator=(const std::string& rhs) {
//     std::cerr << __PRETTY_FUNCTION__ << " stack operator= rhs.size:" << rhs.size() << " k:" << super::k() << "\n";
//     super::operator=(rhs);
//     return *this;
//   }

//   static unsigned int nb_words() { return super::nb_words(super::k()); }
// };

// template<typename T, int CI, 

typedef std::ostream_iterator<char> ostream_char_iterator;
template<typename derived>
inline std::ostream& operator<<(std::ostream& os, const mer_base<derived>& mer) {
  //  char s[static_cast<const derived>(mer).k() + 1];
  char s[mer.k() + 1];
  mer.to_str(s);
  return os << s;
}

typedef std::istream_iterator<char> istream_char_iterator;
template<typename derived>
inline std::istream& operator>>(std::istream& is, mer_base<derived>& mer) {
  if(is.flags() & std::ios::skipws) {
    while(isspace(is.peek())) { is.ignore(1); }
  }

  char buffer[mer.k() + 1];
  is.read(buffer, mer.k());
  if(is.gcount() != mer.k())
    goto error;
  buffer[mer.k()] = '\0';
  if(!mer.from_chars(buffer))
    goto error;
  return is;

 error:
  is.setstate(std::ios::failbit);
  return is;
}

} // namespace mer_dna_ns


typedef mer_dna_ns::mer_base_static<uint32_t> mer_dna32;
typedef mer_dna_ns::mer_base_static<uint64_t> mer_dna64;
#ifdef HAVE_INT128
typedef mer_dna_ns::mer_base_static<unsigned __int128> mer_dna128;
#endif

typedef mer_dna_ns::mer_base_static<uint64_t, mer_dna_ns::mer_len_static<0>, mer_dna_ns::mer_dna_exist<uint64_t> > mer_dna64_stack;
typedef mer_dna64_stack mer_dna_stack;

typedef mer_dna64 mer_dna;

} // namespace jellyfish

#endif
