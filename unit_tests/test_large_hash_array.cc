#include <map>

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

using jellyfish::RectangularBinaryMatrix;
using jellyfish::mer_dna;

// Tuple is {key_len, val_len, reprobe_len}.
class HashArray : public ::testing::TestWithParam< ::std::tr1::tuple<int,int, int> >
{
public:
  static const size_t ary_lsize = 9;
  static const size_t ary_size = (size_t)1 << ary_lsize;
  const int           key_len, val_len, reprobe_limit;
  large_array         ary;

  HashArray() :
    key_len(::std::tr1::get<0>(GetParam())),
    val_len(::std::tr1::get<1>(GetParam())),
    reprobe_limit(::std::tr1::get<2>(GetParam())),
    ary(ary_size, key_len, val_len, (1 << reprobe_limit) - 2)
  { }

  ~HashArray() { }
};

TEST_P(HashArray, OneElement) {
  jellyfish::mer_dna::k(key_len / 2);
  jellyfish::mer_dna m, m2, get_mer;

  SCOPED_TRACE(::testing::Message() << "key_len:" << key_len << " val_len:" << val_len << " reprobe:" << reprobe_limit);

  RectangularBinaryMatrix matrix     = ary.get_matrix();
  RectangularBinaryMatrix inv_matrix = ary.get_inverse_matrix();
  EXPECT_EQ((unsigned int)ary_lsize, matrix.r());
  EXPECT_EQ((unsigned int)key_len, matrix.c());

  for(uint64_t i = 0; i < ary_size; ++i) {
    SCOPED_TRACE(::testing::Message() << "i:" << i);
    // Create mer m so that it will hash to position i
    m.randomize();
    m2 = m;
    m2.set_bits(0, matrix.r(), (uint64_t)i);
    m.set_bits(0, matrix.r(), inv_matrix.times(m2));
    EXPECT_EQ(i, ary.get_matrix().times(m));

    // Add this one element to the hash
    ary.clear();
    bool   is_new = false;
    size_t id     = -1;
    ary.add(m, i, &is_new, &id);
    EXPECT_TRUE(is_new);
    EXPECT_EQ((size_t)i, id);

    // Every position but i in the hash should be empty
    uint64_t val;
    for(size_t j = 0; j < ary_size; ++j) {
      SCOPED_TRACE(::testing::Message() << "j:" << j);
      val = -1;
      ASSERT_EQ(j == id, ary.get_key_val_at_id(j, get_mer, val));
      if(j == id) {
        ASSERT_EQ(m, get_mer);
        ASSERT_EQ((uint64_t)j, val);
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(HashArrayTest, HashArray, ::testing::Combine(::testing::Range(10, 192, 2), // Key lengths
                                                                     ::testing::Range(1, 10),    // Val lengths
                                                                     ::testing::Range(6, 8)      // Reprobe lengths
                                                                     ));

// TEST(HashArray, HundredSmallElements) {
//   typedef jellyfish::invertible_hash::array<uint64_t> inv_array;
//   typedef jellyfish::large_hash::array<jellyfish::mer_dna> large_array;
//   static const size_t   ary_size      = 200;
//   static const uint16_t key_len       = 62;
//   static const uint16_t val_len       = 3;
//   static const uint16_t reprobe_limit = 63;

//   inv_array inv_ary(ary_size, key_len, val_len, reprobe_limit);
//   large_array lar_ary(ary_size, key_len, val_len, reprobe_limit);
//   std::map<uint64_t, uint64_t> map;
//   jellyfish::mer_dna::k(key_len / 2);
//   jellyfish::mer_dna mer;

//   for(int i = 0; i < 100; ++i) {
//     uint64_t k = random_bits(62);
//     inv_ary.add(k, i);
//     mer.set_bits(0, key_len, k);
//     lar_ary.add(mer, i);
//     map[k] = i;
//   }

//   inv_array::iterator it = inv_ary.iterator_all();
//   large_array::iterator lit = lar_ary.iterator_all();
//   int count = 0;
//   while(true) {
//     bool more = it.next();
//     bool lmore = lit.next();
//     EXPECT_EQ(more, lmore);
//     if(!more || !lmore)
//       break;
//     ASSERT_NE(map.end(), map.find(it.key));
//     EXPECT_EQ(map[it.key], it.val);
//     EXPECT_EQ(it.val, lit.val);
//     ++count;
//   }
//   EXPECT_EQ(map.size(), (size_t)count);

// }
}
