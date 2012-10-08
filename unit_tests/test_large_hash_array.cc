#include <map>
#include <vector>

#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>

#include <jellyfish/invertible_hash_array.hpp>
#include <jellyfish/large_hash_array.hpp>
#include <jellyfish/mer_dna.hpp>

void PrintTo(jellyfish::mer_dna& m, ::std::ostream* os) {
  *os << m.to_str();
}

namespace {
typedef jellyfish::invertible_hash::array<uint64_t> inv_array;
typedef jellyfish::large_hash::array<jellyfish::mer_dna> large_array;
typedef std::map<jellyfish::mer_dna, uint64_t> mer_map;

using jellyfish::RectangularBinaryMatrix;
using jellyfish::mer_dna;

// Tuple is {key_len, val_len, reprobe_len}.
class HashArray : public ::testing::TestWithParam< ::std::tr1::tuple<int,int, int> >
{
public:
  static const size_t ary_lsize = 9;
  static const size_t ary_size = (size_t)1 << ary_lsize;
  static const size_t ary_size_mask = ary_size - 1;
  const int           key_len, val_len, reprobe_len, reprobe_limit;
  large_array         ary;

  HashArray() :
    key_len(::std::tr1::get<0>(GetParam())),
    val_len(::std::tr1::get<1>(GetParam())),
    reprobe_len(::std::tr1::get<2>(GetParam())),
    reprobe_limit((1 << reprobe_len) - 2),
    ary(ary_size, key_len, val_len, reprobe_limit)
  { }

  void SetUp() {
    jellyfish::mer_dna::k(key_len / 2);
  }

  ~HashArray() { }
};

TEST_P(HashArray, OneElement) {
  jellyfish::mer_dna m, m2, get_mer;

  SCOPED_TRACE(::testing::Message() << "key_len:" << key_len << " val_len:" << val_len << " reprobe:" << reprobe_limit);

  EXPECT_EQ((unsigned int)ary_lsize, ary.matrix().r());
  EXPECT_EQ((unsigned int)key_len, ary.matrix().c());

  size_t start_pos = random() % (ary_size - bsizeof(uint64_t));
  for(uint64_t i = start_pos; i < start_pos + bsizeof(uint64_t); ++i) {
    SCOPED_TRACE(::testing::Message() << "i:" << i);
    // Create mer m so that it will hash to position i
    m.randomize();
    m2 = m;
    m2.set_bits(0, ary.matrix().r(), (uint64_t)i);
    m.set_bits(0, ary.matrix().r(), ary.inverse_matrix().times(m2));

    // Add this one element to the hash
    ary.clear();
    bool   is_new = false;
    size_t id     = -1;
    ary.add(m, i, &is_new, &id);
    EXPECT_TRUE(is_new);
    EXPECT_EQ((size_t)i, id);

    // Every position but i in the hash should be empty
    uint64_t val;
    for(ssize_t j = -bsizeof(uint64_t); j <= (ssize_t)bsizeof(uint64_t); ++j) {
      SCOPED_TRACE(::testing::Message() << "j:" << j);
      val = -1;
      size_t jd = (start_pos + j) & ary_size_mask;
      ASSERT_EQ(jd == id, ary.get_key_val_at_id(jd, get_mer, val) == large_array::FILLED);
      if(jd == id) {
        ASSERT_EQ(m2, get_mer);
        ASSERT_EQ((uint64_t)jd, val);
      }
    }
  }
}

TEST_P(HashArray, Collisions) {
  static const int nb_collisions = 4;
  std::vector<mer_dna> mers(nb_collisions);
  std::vector<mer_dna> mers2(nb_collisions);
  std::map<mer_dna, uint64_t> map;
  ASSERT_EQ((unsigned int)key_len / 2, mer_dna::k());

  SCOPED_TRACE(::testing::Message() << "key_len:" << key_len << " val_len:" << val_len << " reprobe:" << reprobe_limit);

  mers[0].polyA(); mers2[0].polyA();
  mers[1].polyC(); mers2[1].polyC();
  mers[2].polyG(); mers2[2].polyG();
  mers[3].polyT(); mers2[3].polyT();

  size_t start_pos = random() % (ary_size - bsizeof(uint64_t));
  for(uint64_t i = start_pos; i < start_pos + bsizeof(uint64_t); ++i) {
    SCOPED_TRACE(::testing::Message() << "i:" << i);
    ary.clear();
    map.clear();

    // Add mers that it will all hash to position i
    for(int j = 0; j < nb_collisions; ++j) {
      mers2[j].set_bits(0, ary.matrix().r(), (uint64_t)i);
      mers[j].set_bits(0, ary.matrix().r(), ary.inverse_matrix().times(mers2[j]));
      ary.add(mers[j], 1);
      ++map[mers[j]];
    }

    large_array::iterator it    = ary.iterator_all();
    size_t                count = 0;
    while(it.next()) {
      SCOPED_TRACE(::testing::Message() << "it.key():" << it.key());
      ASSERT_FALSE(map.end() == map.find(it.key()));
      EXPECT_EQ(map[it.key()], it.val());
      ++count;
    }
    EXPECT_EQ(map.size(), count);
  }
}

TEST_P(HashArray, Iterator) {
  static const int nb_elts = 100;
  SCOPED_TRACE(::testing::Message() << "key_len:" << key_len << " val_len:" << val_len << " reprobe:" << reprobe_limit);

  mer_map            map;
  jellyfish::mer_dna mer;

  for(int i = 0; i < nb_elts; ++i) {
    mer.randomize();
    ASSERT_TRUE(ary.add(mer, i));
    map[mer] += i;
  }

  large_array::iterator it = ary.iterator_all();
  int count = 0;
  while(it.next()) {
    mer_map::const_iterator mit = map.find(it.key());
    ASSERT_NE(map.end(), mit);
    SCOPED_TRACE(::testing::Message() << "key:" << it.key());
    EXPECT_EQ(mit->first, it.key());
    EXPECT_EQ(mit->second, it.val());
    ++count;
  }
  EXPECT_EQ(map.size(), (size_t)count);

  for(mer_map::const_iterator it = map.begin(); it != map.end(); ++it) {
    SCOPED_TRACE(::testing::Message() << "key:" << it->first);
    uint64_t val;
    size_t   id;
    EXPECT_TRUE(ary.get_key_id(it->first, &id));
    EXPECT_TRUE(ary.get_val_for_key(it->first, &val));
    EXPECT_EQ(it->second, val);
  }
}

INSTANTIATE_TEST_CASE_P(HashArrayTest, HashArray, ::testing::Combine(::testing::Range(10, 4 * 64, 2), // Key lengths
                                                                     ::testing::Range(1, 10),    // Val lengths
                                                                     ::testing::Range(6, 8)      // Reprobe lengths
                                                                     ));

}
