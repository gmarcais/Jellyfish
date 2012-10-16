#include <gtest/gtest.h>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/mer_dna.hpp>
#include <map>
#include <vector>

namespace {
using jellyfish::mer_dna;
typedef jellyfish::cooperative::hash_counter<mer_dna> hash_counter;

class hash_adder : public thread_exec {
  typedef std::map<mer_dna, uint64_t> map;
  typedef std::vector<map>            maps;

  hash_counter& hash_;
  int           nb_;
  maps          check_;

public:
  hash_adder(hash_counter& hash, int nb, int nb_threads) :
    hash_(hash),
    nb_(nb),
    check_(nb_threads)
  { }
  void start(int id) {
    mer_dna m;
    map&    my_map = check_[id];

    for(int i = 0; i < nb_; ++i) {
      m.randomize();
      hash_.add(m, 1);
      ++my_map[m];
    }

    hash_.done();
  }

  uint64_t val(const mer_dna& m) {
    uint64_t res = 0;
    for(maps::const_iterator it = check_.begin(); it < check_.end(); ++it) {
      map::const_iterator vit = (*it).find(m);
      if(vit != it->end())
        res += vit->second;
    }
    return res;
  }
};

TEST(HashCounterCooperative, SizeDouble) {
  static const int    mer_len    = 35;
  static const int    nb_threads = 5;
  static const int    nb         = 100;
  static const size_t init_size  = 128;
  mer_dna::k(mer_len);
  hash_counter hash(init_size, mer_len * 2, 5, nb_threads);

  EXPECT_EQ(mer_len * 2, hash.key_len());
  EXPECT_EQ(5, hash.val_len());

  hash_adder adder(hash, nb, nb_threads);
  adder.exec_join(nb_threads);

  hash_counter::array::eager_iterator it = hash.ary()->iterator_all();
  while(it.next())
    EXPECT_EQ(adder.val(it.key()), it.val());
  EXPECT_LT((size_t)(nb_threads * nb), hash.size());
}

} // namespace {
