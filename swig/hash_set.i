/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

/*********************************************************************/
/* Proxy class for hash counter: auto size doubling hash on mer_dna. */
/*********************************************************************/
%{
  class HashSet : public jellyfish::cooperative::hash_counter<jellyfish::mer_dna> {
    typedef jellyfish::cooperative::hash_counter<jellyfish::mer_dna> super;
  public:
    HashSet(size_t size, unsigned int nb_threads = 1) : \
    super(size, jellyfish::mer_dna::k() * 2, 0, nb_threads)
      { }

    bool add(const MerDNA& m) {
      bool res;
      size_t id;
      super::set(m, &res, &id);
      return res;
    }
  };
%}

class HashSet {
public:
  HashSet(size_t size, unsigned int nb_threads = 1);
  size_t size() const;
  //  unsigned int nb_threads() const;

  bool add(const MerDNA& m);

  %extend {
    bool get(const MerDNA& m) const { return $self->ary()->has_key(m); }
#ifndef SWIGPERL
    bool __getitem__(const MerDNA& m) const { return $self->ary()->has_key(m); }
#endif
  }
};
