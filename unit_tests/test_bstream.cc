#include <sstream>

#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>

#include <jellyfish/mer_dna.hpp>
#include <jellyfish/bstream.hpp>

namespace {
using jellyfish::mer_dna;

TEST(Bstream, Write) {
  const char text[8] = { 'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd' };
  uint64_t wtext = *(uint64_t*)text;

  std::ostringstream            os(std::ios::out | std::ios::app);
  jellyfish::obstream<uint32_t> bs(os);
  size_t                        bit_len = 0;

  for(int i = 0; i < 50; ++i) {
    size_t rlen = random_bits(5);
    uint32_t word = (uint32_t)(wtext >> (bit_len % 32) & (((uint64_t)1 << 32) - 1));
    bs.write(word, rlen);
    bit_len += rlen;
  }
  bs.align();

  SCOPED_TRACE(::testing::Message() << "bit_len:" << bit_len << " os:" << os.str());
  EXPECT_EQ(4 * (bit_len / 32 + (bit_len % 32 > 0)), os.str().size());
  size_t nb_chars = bit_len / 8;

  for(size_t i = 0; i < nb_chars; ++i) {
    SCOPED_TRACE(::testing::Message() << "i:" << i);
    EXPECT_EQ('a' + (i % 4), os.str()[i]);
  }
}

TEST(Bstream, WriteRead) {
  mer_dna::k(500);
  mer_dna m;
  m.randomize();

  std::ostringstream            os;
  jellyfish::obstream<uint64_t> obs(os);
  std::vector<size_t> rlens;

  size_t bits = 0;
  do {
    size_t rlen = std::min(random_bits(6) + 1, 2 * mer_dna::k() - bits);
    rlens.push_back(rlen);
    obs.write(m.get_bits(bits, rlen), rlen);
    bits += rlen;
  } while(bits < 2 * m.k());
  obs.align();

  EXPECT_EQ(8 * (mer_dna::k() / (4 * sizeof(uint64_t)) + (mer_dna::k() % (4 * sizeof(uint64_t)) > 0)), os.str().size());

  std::istringstream            is(os.str());
  jellyfish::ibstream<uint64_t> ibs(is);

  mer_dna m2;
  bits = 0;
  int i = 0;
  do {
    //    size_t rlen = std::min(random_bits(6) + 1, 2 * mer_dna::k() - bits);
    size_t rlen = rlens[i++];
    m2.set_bits(bits, rlen, ibs.read(rlen));
    bits += rlen;
  } while(bits < 2 * m2.k());
  ibs.align();

  EXPECT_EQ(m, m2);
  EXPECT_TRUE(ibs.stream().good());

  ibs.read(64);
  EXPECT_FALSE(ibs.stream().good());
}
}
