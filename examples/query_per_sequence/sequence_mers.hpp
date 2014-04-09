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

#ifndef __SEQUENCE_MERS_H__
#define __SEQUENCE_MERS_H__

#include <iterator>
#include <jellyfish/mer_dna.hpp>

class sequence_mers : public std::iterator<std::input_iterator_tag, jellyfish::mer_dna> {
public:
  typedef jellyfish::mer_dna mer_type;

private:
  const char*  cseq_;
  const char*  eseq_;
  const bool   canonical_;
  unsigned int filled_;
  mer_type      m_;              // mer
  mer_type      rcm_;            // reverse complement mer

public:
  explicit sequence_mers(bool canonical = false) :
    cseq_(0), eseq_(0), canonical_(canonical), filled_(0)
  { }

  sequence_mers(const sequence_mers& rhs) :
    cseq_(rhs.cseq_), eseq_(rhs.eseq_), canonical_(rhs.canonical_), filled_(rhs.filled_),
    m_(rhs.m_), rcm_(rhs.rcm_)
  { }

  sequence_mers(sequence_mers&& rhs) :
    cseq_(rhs.cseq_), eseq_(rhs.eseq_), canonical_(rhs.canonical_), filled_(rhs.filled_),
    m_(std::move(rhs.m_)), rcm_(std::move(rcm_))
  { }

  void reset(const char* seq, const char* seqe) {
    cseq_   = seq;
    eseq_   = seqe;
    filled_ = 0;
    this->operator++();
  }

  sequence_mers& operator=(const std::string& seq) {
    reset(seq.c_str(), seq.c_str() + seq.size());
    return *this;
  }

  sequence_mers& operator=(const char* seq) {
    reset(seq, seq + strlen(seq));
    return *this;
  }

  bool operator==(const sequence_mers& rhs) const { return cseq_ == rhs.cseq_; }
  bool operator!=(const sequence_mers& rhs) const { return cseq_ != rhs.cseq_; }

  operator void*() const { return (void*)cseq_; }
  const mer_type& operator*() const { return !canonical_ || m_ < rcm_ ? m_ : rcm_; }
  const mer_type* operator->() const { return &this->operator*(); }
  sequence_mers& operator++() {
    if(cseq_ >= eseq_) {
      cseq_ = eseq_ = 0;
      return *this;
    }

    do {
      int code = m_.code(*cseq_++);
      if(code >= 0) {
        m_.shift_left(code);
        if(canonical_)
          rcm_.shift_right(rcm_.complement(code));
        filled_ = std::min(filled_ + 1, jellyfish::mer_dna::k());
      } else
        filled_ = 0;
    } while(filled_ < jellyfish::mer_dna::k() && cseq_ < eseq_);

    return *this;
  }
  sequence_mers operator++(int) {
    sequence_mers res(*this);
    ++*this;
    return res;
  }
};

#endif /* __SEQUENCE_MERS_H__ */
