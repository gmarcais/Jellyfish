/*********************************************************************/
/* Proxy class for hash counter: auto size doubling hash on mer_dna. */
/*********************************************************************/
%{
  class HashCounter : public jellyfish::cooperative::hash_counter<jellyfish::mer_dna> {
    typedef jellyfish::cooperative::hash_counter<jellyfish::mer_dna> super;
  public:
    HashCounter(size_t size, unsigned int val_len, unsigned int nb_threads = 1) : \
    super(size, jellyfish::mer_dna::k() * 2, val_len, nb_threads)
      { }

    bool add(const MerDNA& m, const int& x) {
      bool res;
      size_t id;
      super::add(m, x, &res, &id);
      return res;
    }

  };
%}

// Typemaps to return nil/undef/None if the mer asked for is not in
// the hash
%typemap(in, numinputs=0) std::pair<bool, uint64_t>* COUNT (std::pair<bool, uint64_t> tmp) {
  $1 = &tmp;
 }
%typemap(argout) std::pair<bool, uint64_t>* COUNT {
  if(($1)->first) {
    SWIG_Object o = SWIG_From(unsigned long)(($1)->second);
    %append_output(o);
  } else {
    %append_output(VOID_Object);
  }
 }

class HashCounter {
public:
  HashCounter(size_t size, unsigned int val_len, unsigned int nb_threads = 1);
  size_t size() const;
  unsigned int val_len() const;
  //  unsigned int nb_threads() const;

  bool add(const MerDNA& m, const int& x);
  bool update_add(const MerDNA&, const int&);

  %extend {
    void get(const MerDNA& m, std::pair<bool, uint64_t>* COUNT) const {
      COUNT->first = $self->ary()->get_val_for_key(m, &COUNT->second);
    }
#ifndef SWIGPERL
    void __getitem__(const MerDNA& m, std::pair<bool, uint64_t>* COUNT) const {
      COUNT->first = $self->ary()->get_val_for_key(m, &COUNT->second);
    }
#endif
  }
};
