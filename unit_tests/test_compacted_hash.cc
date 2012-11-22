#include <iostream>
#include <sstream>
#include <set>

#include <gtest/gtest.h>

#include <jellyfish/mer_dna.hpp>
#include <jellyfish/compacted_hash.hpp>

namespace {
using jellyfish::mer_dna;
using jellyfish::RectangularBinaryMatrix;
typedef jellyfish::compacted_hash::writer<mer_dna> sorted_writer;
typedef jellyfish::compacted_hash::stream_iterator<mer_dna> sorted_reader;

struct mer_pos {
  mer_dna  m;
  uint64_t pos;

  bool operator<(const mer_pos& rhs) const {
    return pos == rhs.pos ? m < rhs.m : pos < rhs.pos;
  }
};
typedef std::set<mer_pos> mer_set;

// Parameters: k-mer length, length of size of hash, hash usage,
// offset field length
typedef ::std::tr1::tuple<int, int, float, int> parameters;
using ::std::tr1::get;

class CompactedHash : public ::testing::TestWithParam<parameters> { };

TEST_P(CompactedHash, WriteRead) {
  mer_dna::k(get<0>(GetParam()));
  const int size_len = get<1>(GetParam()); // Length of size in bits
  const int nb_mers = jellyfish::bitmask<uint64_t>(size_len) * get<2>(GetParam());
  const uint16_t off_len = get<3>(GetParam());
  const uint16_t val_len = 1;
  const uint16_t block_len = 100;

  RectangularBinaryMatrix mat(size_len, 2 * mer_dna::k());
  RectangularBinaryMatrix inv(mat.randomize_pseudo_inverse());

  mer_set set;
  mer_pos mp;
  for(int i = 0; i < nb_mers; ++i) {
    while(true) {
      mp.m.randomize();
      mp.pos = mat.times(mp.m);
      std::pair<mer_set::iterator, bool> res = set.insert(mp);
      if(res.second) // Don't insert same k-mer twice
        break;
    }
  }

  std::ostringstream os;
  {
    sorted_writer writer(os, 2 * mer_dna::k(), 2 * mer_dna::k() - size_len,
                         off_len, val_len, block_len);

    EXPECT_EQ(2 * mer_dna::k(), writer.key_len());
    EXPECT_EQ(2 * mer_dna::k() - size_len, writer.short_key_len());
    EXPECT_EQ(off_len, writer.off_len());
    EXPECT_EQ(val_len, writer.val_len());

    uint64_t prev_pos = 0;
    int count = 0;
    for(mer_set::const_iterator it = set.begin(); it != set.end(); prev_pos = it->pos, ++it, ++count) {
      ASSERT_LE(prev_pos, it->pos);
      writer.write(it->m, it->m.get_bits(0, 1), it->pos);
    }
  } // Writer destroy -> should pad output

  ASSERT_TRUE(os.str().size() % sizeof(uint64_t) == 0);

  std::istringstream is(os.str());
  sorted_reader reader(is, 2 * mer_dna::k(), 2 * mer_dna::k() - size_len,
                       off_len, val_len, inv, block_len);
  int      count    = 0;
  uint64_t prev_pos = 0;
  for(mer_set::const_iterator it = set.begin(); it != set.end(); ++it, ++count) {
    SCOPED_TRACE(::testing::Message() << "count:" << count);
    ASSERT_TRUE(reader.next());
    EXPECT_LE(prev_pos, reader.id());
    EXPECT_EQ(it->m, reader.key());
    prev_pos = reader.id();
  }
  EXPECT_EQ(nb_mers, count);
  EXPECT_FALSE(reader.next());
}

INSTANTIATE_TEST_CASE_P(CompactedHashTest, CompactedHash,
                        ::testing::Combine(::testing::Range(8, 192, 11),
                                           ::testing::Range(5, 11),
                                           ::testing::Values(0.33, 0.5, 0.8),
                                           ::testing::Values(1, 2, 3)));
}
