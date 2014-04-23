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
  };
%}

class HashCounter {
public:
  HashCounter(size_t size, unsigned int val_len, unsigned int nb_threads = 1);
  size_t size() const;
  unsigned int val_len() const;
  //  unsigned int nb_threads() const;
  };
