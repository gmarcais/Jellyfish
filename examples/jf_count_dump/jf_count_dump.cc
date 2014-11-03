#include <iostream>

#include <jellyfish/mer_dna.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/stream_manager.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_iterator.hpp>

typedef jellyfish::cooperative::hash_counter<jellyfish::mer_dna>                  mer_hash_type;
typedef jellyfish::mer_overlap_sequence_parser<jellyfish::stream_manager<char**>> sequence_parser_type;
typedef jellyfish::mer_iterator<sequence_parser_type, jellyfish::mer_dna>         mer_iterator_type;


class mer_counter : public jellyfish::thread_exec {
  mer_hash_type&                    mer_hash_;
  jellyfish::stream_manager<char**> streams_;
  sequence_parser_type              parser_;
  const bool                        canonical_;

public:
  mer_counter(int nb_threads, mer_hash_type& mer_hash,
              char** file_begin, char** file_end,
              bool canonical)
    : mer_hash_(mer_hash)
    , streams_(file_begin, file_end)
    , parser_(jellyfish::mer_dna::k(), streams_.nb_streams(), 3 * nb_threads, 4096, streams_)
    , canonical_(canonical)
  { }

  virtual void start(int thid) {
    mer_iterator_type mers(parser_, canonical_);

    for( ; mers; ++mers)
      mer_hash_.add(*mers, 1);
    mer_hash_.done();
  }
};


int main(int argc, char *argv[]) {
  // Parameters that are hard coded. Most likely some of those should
  // be switches to this program.
  jellyfish::mer_dna::k(25); // Set length of mers (k=25)
  const uint64_t hash_size    = 10000000; // Initial size of hash.
  const uint32_t num_reprobes = 126;
  const uint32_t num_threads  = 16; // Number of concurrent threads
  const uint32_t counter_len  = 7;  // Minimum length of counting field
  const bool     canonical    = true; // Use canonical representation

  // create the hash
  mer_hash_type mer_hash(hash_size, jellyfish::mer_dna::k()*2, counter_len, num_threads, num_reprobes);


  // count the kmers
  mer_counter counter(num_threads, mer_hash, argv + 1, argv + argc, canonical);
  counter.exec_join(num_threads);

  const auto jf_ary = mer_hash.ary();

  // Display value for some random k-mer
  uint64_t           val = 0;
  jellyfish::mer_dna random_mer;
  random_mer.randomize();
  random_mer.canonicalize();
  if(jf_ary->get_val_for_key(random_mer, &val)) {
    std::cout << random_mer << ' ' << val << '\n';
  } else {
    std::cout << random_mer << " not present in hash\n";
  }

  // Dump all the k-mers on stdout
  const auto end = jf_ary->end();
  for(auto it = jf_ary->begin(); it != end; ++it) {
    auto& key_val = *it;
    std::cout << key_val.first << ' ' << key_val.second << '\n';
  }

  return 0;
}
