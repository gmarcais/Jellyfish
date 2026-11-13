/* SuperRead pipeline
 * Copyright (C) 2012  Genome group at University of Maryland.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <jellyfish/mer_dna.hpp>
#include <unit_tests/test_main.hpp>

namespace {
using namespace jellyfish;

const std::string short_mer("ACGTTGGCCAA");
const std::string mid_mer("AAGACTTAGAATCAGCTAAGGTAACTTACGTAGAATATAGA");
const std::string long_mer("AAGACTTAGAATCAGCTAAGGTAACTTACGTAGAATATAG"
                           "AAGACGGCGCCTATCCCAACCTCCATTTGCTGACGCCCTC"
                           "GTAACCGTGCTGCGAGGTTACTCTATACTGA");
const std::string huge_mer("CAGGACACAATACGTCGATCCAATCCCCGACGTGAGAGTT"
                           "TAACGCTAATCTTGATCCATTACAAGTATAGATATTTCGG"
                           "GCGCCACGGGGAAACTTGCCCTATGTTCGAGTCCGCCACC"
                           "GCGGCACCAGCCTTTGTGTGACGGCCACAAAGGGTAAAAG"
                           "ATGTTGTCTCGCGCCGGCGTGCGGTCTTTACCAGATCCTT"
                           "GAGCGGCCTAGAAAGTTCGTACCAGTTCGACTGAAACAAG"
                           "ACAACGGATCGCCCACGGTCATCACACGCCCACGCACGGG"
                           "GTCGGGTTGGTATATTCAACCTCGAGTTAAACGT");
const std::string* test_mers[4] = {
  &short_mer, &mid_mer, &long_mer, &huge_mer
};

TEST(MerDNASimple, InitSize64) {
  struct sinfo {
    unsigned int k;
    unsigned int nb_words;
    unsigned int nb_msb;
    uint64_t     msw;
    unsigned int lshift;
  };
  sinfo ary[5] = {
    { 5, 1, 10, (uint64_t)0x3ff, 8 }, { 32, 1, 64, (uint64_t)-1, 62 },
    { 33, 2, 2, (uint64_t)0x3, 0 }, { 64, 2, 64, (uint64_t)-1, 62 },
    { 65, 3, 2, (uint64_t)0x3, 0 }
  };
  typedef mer_dna_ns::mer_base_dynamic<uint64_t> mer64;
  for(size_t i = 0; i < sizeof(ary) / sizeof(sinfo); ++i) {
    mer64 m(ary[i].k);
    EXPECT_EQ(ary[i].k, m.k());
    EXPECT_EQ(ary[i].nb_words, m.nb_words());
    EXPECT_EQ(ary[i].nb_msb, m.nb_msb());
    EXPECT_EQ(ary[i].lshift, m.lshift());
  }
}

#ifdef HAVE_INT128
TEST(MerDNASimple, InitSize128) {
  struct sinfo {
    unsigned int      k;
    unsigned int      nb_words;
    unsigned int      nb_msb;
    unsigned __int128 msw;
    unsigned int      lshift;
  };
  sinfo ary[5] = {
    { 5, 1, 10, (unsigned __int128)0x3ff, 8 },
    { 32, 1, 64, (unsigned __int128)0xffffffffffffffffUL, 62 },
    { 33, 1, 66, (unsigned __int128)0xffffffffffffffffUL | ((unsigned __int128)0x3 << 64), 64 },
    { 64, 1, 128, (unsigned __int128)-1, 126 },
    { 65, 2, 2, (unsigned __int128)0x3, 0 }
  };
  typedef mer_dna_ns::mer_base_dynamic<unsigned __int128> mer128;
  for(size_t i = 0; i < sizeof(ary) / sizeof(sinfo); ++i) {
    mer128 m(ary[i].k);
    EXPECT_EQ(ary[i].k, m.k());
    EXPECT_EQ(ary[i].nb_words, m.nb_words());
    EXPECT_EQ(ary[i].nb_msb, m.nb_msb());
    EXPECT_EQ(ary[i].lshift, m.lshift());
  }
}
#endif

TEST(MerDNASimple, Codes) {
  EXPECT_EQ(mer_dna::CODE_A, mer_dna::code('A'));
  EXPECT_EQ(mer_dna::CODE_A, mer_dna::code('a'));
  EXPECT_EQ(mer_dna::CODE_C, mer_dna::code('C'));
  EXPECT_EQ(mer_dna::CODE_C, mer_dna::code('c'));
  EXPECT_EQ(mer_dna::CODE_G, mer_dna::code('G'));
  EXPECT_EQ(mer_dna::CODE_G, mer_dna::code('g'));
  EXPECT_EQ(mer_dna::CODE_T, mer_dna::code('T'));
  EXPECT_EQ(mer_dna::CODE_T, mer_dna::code('t'));
  EXPECT_FALSE(mer_dna::not_dna(mer_dna::CODE_A));
  EXPECT_FALSE(mer_dna::not_dna(mer_dna::CODE_C));
  EXPECT_FALSE(mer_dna::not_dna(mer_dna::CODE_G));
  EXPECT_FALSE(mer_dna::not_dna(mer_dna::CODE_T));

  for(int c = 0; c < 256; ++c) {
    switch((char)c) {
    case 'A': case 'a':
    case 'C': case 'c':
    case 'G': case 'g':
    case 'T': case 't':
      EXPECT_FALSE(mer_dna::not_dna(mer_dna::code(c)));
      break;
    default:
      EXPECT_TRUE(mer_dna::not_dna(mer_dna::code(c)));
      break;
    }
  }
}

TEST(MerDNASimple, RandomizeAdvances) {
  typedef mer_dna_ns::mer_base_dynamic<uint64_t> Mer;
  Mer first(64), second(64);
  first.randomize();
  second.randomize();
  EXPECT_NE(first, second);
}

TEST(MerDNASimple, SetBits) {
  mer_dna::k(100);
  static const int pattern_len = 10;
  uint64_t pattern = ::random_bits(pattern_len); // Create a random pattern
  mer_dna even_pattern, odd_pattern;             // And the corresponding mers
  for(int i = pattern_len - 2; i >= 0; i -= 2) {
    even_pattern.shift_left((int)((pattern >> i) & 0x3));
    odd_pattern.shift_left((int)((pattern >> (i + 1)) & 0x3));
  }
  odd_pattern.shift_left((int)((pattern << 1) & 0x3));

  mer_dna mer;
  for(int i = 0; i <= (int)(mer_dna::k() - pattern_len / 2); ++i, even_pattern.shift_left('A'), odd_pattern.shift_left('A')) {
    // Even
    mer.polyA();
    EXPECT_EQ(std::string(mer_dna::k(), 'A'), mer.to_str());
    mer.set_bits(2 * i, pattern_len, pattern);
    EXPECT_EQ(pattern, mer.get_bits(2 * i, pattern_len));
    EXPECT_EQ(even_pattern, mer);
    EXPECT_EQ(even_pattern.to_str(), mer.to_str());
    // Odd
    mer.polyA();
    mer.set_bits(2 * i + 1, pattern_len, pattern);
    if(i < (int)(mer_dna::k() - pattern_len / 2))
      EXPECT_EQ(pattern, mer.get_bits(2 * i + 1, pattern_len));
    else // On the largest value of i, one bit may have fallen off the end of the mer
      EXPECT_EQ(pattern & (((uint64_t)1 << (pattern_len - 1)) - 1), mer.get_bits(2 * i + 1, pattern_len));
    EXPECT_EQ(odd_pattern, mer);
    EXPECT_EQ(odd_pattern.to_str(), mer.to_str());
  }
}

TEST(MerDNASimple, Shifts) {
  for(int i = 1; i < 100; ++i) {
    mer_dna::k(i);
    mer_dna m;

    m.randomize(prg);
    const int c = ::random_bits(2);
    mer_dna rm(m);
    rm.shift_right(c);
    mer_dna lm(m);
    lm.shift_left(c);

    EXPECT_EQ(c, rm.base(mer_dna::k() - 1).code());
    EXPECT_EQ(c, lm.base(0).code());
    for(unsigned int j = 0; j < mer_dna::k() - 1; ++j) {
      EXPECT_EQ(m.base(j + 1).code(), rm.base(j).code());
      EXPECT_EQ(m.base(j).code(), lm.base(j + 1).code());
    }
  }
} // MerDNASimple.Shifts


bool simple_homolymer_test(const mer_dna& m) {
  mer_dna cm(m);
  cm.shift_right(m.base(0).code());
  return cm == m;
}

TEST(MerDNASimple, Homopolymer) {
  std::uniform_int_distribution<unsigned int> base_index;
  for(int i = 1; i < 256; ++i) {
    SCOPED_TRACE(::testing::Message() << "i:" << i);
    mer_dna::k(i);
    mer_dna m;

    for(int j = 0; j < 10; ++j) {
      m.randomize(prg);
      EXPECT_EQ(simple_homolymer_test(m), m.is_homopolymer());
    }

    m.polyA();
    EXPECT_TRUE(simple_homolymer_test(m));
    EXPECT_TRUE(m.is_homopolymer());
    if(i > 1) { // i == 1 all mers are homopolymers by definition
      const auto base_range = decltype(base_index)::param_type(0, i - 1);
      m.base(base_index(prg, base_range)) = 'T';
      if(simple_homolymer_test(m) || m.is_homopolymer())
        std::cerr << m << "\n";
      EXPECT_FALSE(simple_homolymer_test(m));
      EXPECT_FALSE(m.is_homopolymer());
    }

    m.polyC();
    EXPECT_TRUE(simple_homolymer_test(m));
    EXPECT_TRUE(m.is_homopolymer());
    m.polyG();
    EXPECT_TRUE(simple_homolymer_test(m));
    EXPECT_TRUE(m.is_homopolymer());
    m.polyT();
    EXPECT_TRUE(simple_homolymer_test(m));
    EXPECT_TRUE(m.is_homopolymer());
  }
} // MerDNASimple.Homopolymer

TEST(MerDNASimple, Comparators) {
  mer_dna::k(151);
  mer_dna ma, mc, mg, mt;
  ma.polyA();
  mc.polyC();
  mg.polyG();
  mt.polyT();

  mer_dna cma(ma), cmc(mc), cmg(mg), cmt(mt);

  ASSERT_TRUE(ma < mc); ASSERT_FALSE(mc < ma);
  ASSERT_TRUE(ma < mg); ASSERT_FALSE(mg < ma);
  ASSERT_TRUE(ma < mt); ASSERT_FALSE(mt < ma);
  ASSERT_TRUE(mc < mg); ASSERT_FALSE(mg < mc);
  ASSERT_TRUE(mc < mt); ASSERT_FALSE(mt < mc);
  ASSERT_TRUE(mg < mt); ASSERT_FALSE(mt < mg);

  ASSERT_FALSE(ma < ma);
  ASSERT_FALSE(mc < mc);
  ASSERT_FALSE(mg < mg);
  ASSERT_FALSE(mt < mt);

  std::map<mer_dna, int> map;
  EXPECT_EQ(1, ++map[ma]);
  EXPECT_EQ(1, ++map[mc]);
  EXPECT_EQ(1, ++map[mg]);
  EXPECT_EQ(1, ++map[mt]);
  EXPECT_EQ(2, ++map[cma]);
  EXPECT_EQ(2, ++map[cmc]);
  EXPECT_EQ(2, ++map[cmg]);
  EXPECT_EQ(2, ++map[cmt]);

  mer_dna m1, m2;
  for(int i = 0; i < 1000; ++i) {
    m1.randomize(prg);
    m2.randomize(prg);
    SCOPED_TRACE(::testing::Message() << "m1:" << m1 << " m2:" << m2);
    ASSERT_FALSE(m1 == m2); // Very small probability to fail (k == 151)

    EXPECT_TRUE(m1 == m1);
    EXPECT_FALSE(m1 < m1);
    EXPECT_FALSE(m1 > m1);
    EXPECT_TRUE(m1 <= m1);
    EXPECT_TRUE(m1 >= m1);

    EXPECT_TRUE(m2 == m2);
    EXPECT_FALSE(m2 < m2);
    EXPECT_FALSE(m2 > m2);
    EXPECT_TRUE(m2 <= m2);
    EXPECT_TRUE(m2 >= m2);

    EXPECT_EQ(m1.to_str().compare(m2.to_str()) < 0, m1 < m2); // Comparison is lexicographic

    EXPECT_TRUE(m1 < m2 || m2 < m1);
    EXPECT_TRUE(m1 <= m2 || m2 <= m1);
    EXPECT_FALSE(m1 <= m2 && m2 <= m1);
    EXPECT_TRUE(m1 > m2 || m2 > m1);
    EXPECT_TRUE(m1 >= m2 || m2 >= m1);
    EXPECT_FALSE(m1 >= m2 && m2 >= m1);

    EXPECT_NE(m1 < m2, m1 >= m2);
    EXPECT_NE(m1 < m2, m1 > m2);
  }
}

TEST(MerDNASimple, IO) {
  std::stringstream buffer;
  std::uniform_int_distribution<unsigned int> mer_length(1, 512);

  for(int i = 0; i < 10000; ++i) {
    buffer.clear();
    SCOPED_TRACE(::testing::Message() << "i:" << i);
    mer_dna::k(mer_length(prg));
    mer_dna m1, m2;
    m1.randomize(prg);
    buffer << m1;
    buffer >> m2;
    EXPECT_EQ(m1, m2);
  }
}

TEST(MerDNASimple, MultipleSize) {
  typedef jellyfish::mer_dna_ns::mer_base_static<uint64_t, 1> mer_dna1;
  typedef jellyfish::mer_dna_ns::mer_base_static<uint64_t, 2> mer_dna2;

  mer_dna::k(10);
  mer_dna1::k(50);
  mer_dna2::k(100);
  EXPECT_EQ((unsigned int)10, mer_dna::k());
  EXPECT_EQ(0, mer_dna::class_index);
  EXPECT_EQ((unsigned int)50, mer_dna1::k());
  EXPECT_EQ(1, mer_dna1::class_index);
  EXPECT_EQ((unsigned int)100, mer_dna2::k());
  EXPECT_EQ(2, mer_dna2::class_index);
}

// Value Type Container class
template <typename T, int N>
class VTC {
public:
  typedef T Type;
  static const int test_id = N;
};
template <typename T, int N>
const int VTC<T, N>::test_id;

template<typename VT>
class MerDNA : public ::testing::Test {
public:
  typedef typename VT::Type Type;
  void SetUp() {
    Type::k(GetParam().size());
  }
  const std::string& GetParam() const {
    return *test_mers[VT::test_id];
  }
};
template<typename T>
struct StackMers {
  typedef mer_dna_ns::mer_base_stack<T, 32> storage;
  typedef mer_dna_ns::mer_base_dynamic<T, storage> dynamic;
  typedef mer_dna_ns::mer_base_static<T, 0, storage> shared;
};

#define MER_DNA_CASES(Type) VTC<Type, 0>, VTC<Type, 1>, VTC<Type, 2>, VTC<Type, 3>
typedef ::testing::Types<
    MER_DNA_CASES(mer_dna_ns::mer_base_dynamic<uint32_t>),
    MER_DNA_CASES(mer_dna_ns::mer_base_dynamic<uint64_t>),
    MER_DNA_CASES(mer_dna_ns::mer_base_static<uint32_t>),
    MER_DNA_CASES(mer_dna_ns::mer_base_static<uint64_t>),
    MER_DNA_CASES(StackMers<uint32_t>::dynamic),
    MER_DNA_CASES(StackMers<uint64_t>::dynamic),
    MER_DNA_CASES(StackMers<uint32_t>::shared),
    MER_DNA_CASES(StackMers<uint64_t>::shared)
#ifdef HAVE_INT128
    , MER_DNA_CASES(mer_dna_ns::mer_base_dynamic<unsigned __int128>),
    MER_DNA_CASES(mer_dna_ns::mer_base_static<unsigned __int128>),
    MER_DNA_CASES(StackMers<unsigned __int128>::dynamic),
    MER_DNA_CASES(StackMers<unsigned __int128>::shared)
#endif
    > MerDNATypes;
#undef MER_DNA_CASES
TYPED_TEST_CASE(MerDNA, MerDNATypes);

TYPED_TEST(MerDNA, ZeroInitialization) {
  typename TypeParam::Type m(this->GetParam().size());
  EXPECT_EQ(std::string(m.k(), 'A'), m.to_str());
  for(unsigned int i = 0; i < m.nb_words(); ++i)
    EXPECT_EQ(0U, m.word(i));
}

TYPED_TEST(MerDNA, InitFromWords) {
  typedef typename TypeParam::Type Mer;
  typedef typename Mer::base_type Word;
  std::vector<Word> words(Mer::nb_words(this->GetParam().size()),
                          std::numeric_limits<Word>::max());
  Mer m(this->GetParam().size(), words);
  EXPECT_EQ(std::string(m.k(), 'T'), m.to_str());
  EXPECT_EQ(m.msw(), m.word(m.nb_words() - 1));
}

TYPED_TEST(MerDNA, BoundedFromChars) {
  typedef typename TypeParam::Type Mer;
  const std::string& sequence = this->GetParam();
  Mer copy(sequence.size());
  EXPECT_TRUE(copy.from_chars(sequence.begin(), sequence.end()));
  EXPECT_EQ(sequence, copy.to_str());
  EXPECT_TRUE(copy.from_chars(sequence.data(), sequence.data() + sequence.size()));
  EXPECT_EQ(sequence, copy.to_str());
  EXPECT_FALSE(copy.from_chars(sequence.begin(), sequence.end() - 1));
  EXPECT_FALSE(copy.from_chars(sequence.data(), sequence.data() + sequence.size() - 1));
}

TYPED_TEST(MerDNA, StreamRoundTrip) {
  typedef typename TypeParam::Type Mer;
  Mer original(this->GetParam());
  Mer copy(original.k());
  std::stringstream stream;
  stream << original;
  stream >> copy;
  EXPECT_EQ(original, copy);
  EXPECT_FALSE(stream.fail());
}

TYPED_TEST(MerDNA, StreamLeavesFollowingFields) {
  typedef typename TypeParam::Type Mer;
  Mer original(this->GetParam());
  Mer copy(original.k());
  std::stringstream stream;
  stream << " \t" << original << " 37\n" << original << " 81";
  unsigned int count = 0;
  stream >> copy >> count;
  EXPECT_FALSE(stream.fail());
  EXPECT_EQ(original, copy);
  EXPECT_EQ(37U, count);
  stream >> copy >> count;
  EXPECT_FALSE(stream.fail());
  EXPECT_EQ(original, copy);
  EXPECT_EQ(81U, count);
}

TYPED_TEST(MerDNA, StreamRejectsIncompleteOrInvalidInput) {
  typedef typename TypeParam::Type Mer;
  Mer copy(this->GetParam().size());
  std::stringstream truncated(this->GetParam().substr(0, copy.k() - 1));
  truncated >> copy;
  EXPECT_TRUE(truncated.fail());
  EXPECT_TRUE(truncated.eof());

  std::string sequence(this->GetParam());
  sequence[copy.k() / 2] = 'N';
  std::stringstream invalid(sequence);
  invalid >> copy;
  EXPECT_TRUE(invalid.fail());

  sequence[copy.k() / 2] = ' ';
  std::stringstream internal_space(sequence);
  internal_space >> copy;
  EXPECT_TRUE(internal_space.fail());

  std::stringstream leading_space(" " + this->GetParam());
  leading_space >> std::noskipws >> copy;
  EXPECT_TRUE(leading_space.fail());
}

TYPED_TEST(MerDNA, BitFieldBoundaries) {
  typedef typename TypeParam::Type Mer;
  typedef typename Mer::base_type Word;
  Mer m(this->GetParam().size());
  const unsigned int bits = 8 * sizeof(Word);
  if(m.nb_words() > 1) {
    m.set_bits(bits - 1, 2, 3);
    EXPECT_EQ(static_cast<Word>(3), m.get_bits(bits - 1, 2));
    EXPECT_EQ(static_cast<Word>(1) << (bits - 1), m.word(0));
    EXPECT_EQ(static_cast<Word>(1), m.word(1));
  }
  std::string expected(m.k(), 'A');
  m.polyA();
  for(unsigned int i = 0; i < m.k(); ++i) {
    m.set_bits(2 * i, 2, i % 4);
    expected[m.k() - 1 - i] = "ACGT"[i % 4];
  }
  EXPECT_EQ(expected, m.to_str());
}

TYPED_TEST(MerDNA, InitFromStr) {
  typename TypeParam::Type m(this->GetParam());
  EXPECT_EQ(this->GetParam().size(), m.k());
  EXPECT_EQ(this->GetParam(), m.to_str());
}

TYPED_TEST(MerDNA, ShiftLeft) {
  typename TypeParam::Type m(this->GetParam().size());
  m.polyA();
  int inserted = 0;
  for(std::string::const_iterator it = this->GetParam().begin(); it != this->GetParam().end(); ++it, ++inserted) {
    m.shift_left(*it);

    int check = inserted;
    for(std::string::const_iterator cit = this->GetParam().begin(); check >= 0; ++cit, --check)
      EXPECT_EQ(*cit, (char)m.base(check));
  }
  EXPECT_EQ(this->GetParam(), m.to_str());
}

TYPED_TEST(MerDNA, ShiftRight) {
  typename TypeParam::Type m(this->GetParam().size());
  m.polyA();
  int inserted = 0;
  for(std::string::const_reverse_iterator it = this->GetParam().rbegin(); it != this->GetParam().rend(); ++it, ++inserted) {
    m.shift_right(*it);

    int check = inserted;
    for(std::string::const_reverse_iterator cit = this->GetParam().rbegin(); check >= 0; ++cit, --check)
      EXPECT_EQ(*cit, (char)m.base(m.k() - 1 - check));
  }
  EXPECT_EQ(this->GetParam(), m.to_str());
}

TYPED_TEST(MerDNA, Equality) {
  typename TypeParam::Type m1(this->GetParam());
  typename TypeParam::Type m2(this->GetParam().size());

  char str[this->GetParam().size() + 1];
  str[this->GetParam().size()] = '\0';
  memset(str, 'A', this->GetParam().size());
  m2.polyA();
  EXPECT_STREQ(str, m2.to_str().c_str());
  memset(str, 'C', this->GetParam().size());
  m2.polyC();
  EXPECT_STREQ(str, m2.to_str().c_str());
  memset(str, 'G', this->GetParam().size());
  m2.polyG();
  EXPECT_STREQ(str, m2.to_str().c_str());
  memset(str, 'T', this->GetParam().size());
  m2.polyT();
  EXPECT_STREQ(str, m2.to_str().c_str());


  int i = 1;
  for(std::string::const_iterator it = this->GetParam().begin(); it < this->GetParam().end(); ++it, ++i) {
    sprintf(str + this->GetParam().size() - i, "%.*s", i, this->GetParam().c_str());
    typename TypeParam::Type m(str);
    EXPECT_STREQ(str, m.to_str().c_str());
    m2.shift_left(*it);
    EXPECT_EQ(m, m2);
  }
  EXPECT_TRUE(m1 == m2);
  EXPECT_FALSE(m1 != m2);
  EXPECT_EQ(m1.to_str(), m2.to_str());

  // typename TypeParam::Type m3(this->GetParam());
  // m3[0] = 0;
  // EXPECT_FALSE(m1 == m3);

  // typename TypeParam::Type m4(this->GetParam().size() + 1);
  // EXPECT_FALSE(m1 == m4);
  // typename TypeParam::Type m5(this->GetParam().size() - 1);
  // EXPECT_FALSE(m1 == m5);

}

TYPED_TEST(MerDNA, Copy) {
  typename TypeParam::Type m1(this->GetParam());
  typename TypeParam::Type m2(m1);
  typename TypeParam::Type m3(this->GetParam().size());
  typedef typename TypeParam::Type Mer;
  Mer from_base(static_cast<const mer_dna_ns::mer_base<Mer>&>(m1));
  EXPECT_EQ(m1, from_base);
  EXPECT_NE(m1.data(), from_base.data());
  m3 = m1;
  m2 = m2;

  EXPECT_NE(m1.data(), m2.data());
  EXPECT_NE(m1.data(), m3.data());
  EXPECT_TRUE(m1 == m2);
  EXPECT_TRUE(m2 == m3);
  EXPECT_TRUE(m3 == m1);
  m1.shift_left('A');
  EXPECT_TRUE(!(m1 == m2));
  EXPECT_TRUE(!(m1 == m3));
}

TYPED_TEST(MerDNA, MoveConstruction) {
  typedef typename TypeParam::Type Mer;
  typedef typename Mer::base_type Word;
  static_assert(std::is_nothrow_move_constructible<Mer>::value,
                "Built-in storage must allow non-throwing mer moves");
  Mer source(this->GetParam());
  const Word* buffer = source.data();
  Mer moved(std::move(source));
  EXPECT_EQ(this->GetParam(), moved.to_str());
  EXPECT_EQ(this->GetParam().size(), moved.k());
  if(std::is_base_of<mer_dna_ns::mer_base_heap<Word>, Mer>::value) {
    EXPECT_EQ(buffer, moved.data());
    EXPECT_EQ(nullptr, source.data());
  } else {
    EXPECT_NE(buffer, moved.data());
    EXPECT_EQ(this->GetParam(), source.to_str());
  }
  source = moved;
  EXPECT_EQ(moved, source);
  EXPECT_NE(moved.data(), source.data());
}

TYPED_TEST(MerDNA, MoveAssignment) {
  typedef typename TypeParam::Type Mer;
  typedef typename Mer::base_type Word;
  Mer source(this->GetParam());
  Mer destination(source.k());
  const Word* buffer = source.data();
  destination = std::move(source);
  EXPECT_EQ(this->GetParam(), destination.to_str());
  if(std::is_base_of<mer_dna_ns::mer_base_heap<Word>, Mer>::value) {
    EXPECT_EQ(buffer, destination.data());
  } else {
    EXPECT_NE(buffer, destination.data());
    EXPECT_EQ(this->GetParam(), source.to_str());
  }
  Mer& same = destination;
  destination = std::move(same);
  EXPECT_EQ(this->GetParam(), destination.to_str());
  source = std::move(destination);
  EXPECT_EQ(this->GetParam(), source.to_str());
}

TYPED_TEST(MerDNA, VectorRelocation) {
  typedef typename TypeParam::Type Mer;
  typedef typename Mer::base_type Word;
  std::vector<Mer> mers;
  mers.reserve(1);
  mers.emplace_back(this->GetParam());
  const Word* buffer = mers[0].data();
  mers.reserve(2);
  EXPECT_EQ(this->GetParam(), mers[0].to_str());
  if(std::is_base_of<mer_dna_ns::mer_base_heap<Word>, Mer>::value) {
    EXPECT_EQ(buffer, mers[0].data());
  }
}

TYPED_TEST(MerDNA, VectorCopyInsertion) {
  typedef typename TypeParam::Type Mer;
  std::vector<Mer> mers;
  mers.reserve(3);
  mers.emplace_back(this->GetParam());
  mers.push_back(mers.front());
  Mer inserted(this->GetParam());
  inserted.polyT();
  mers.insert(mers.begin() + 1, inserted);
  EXPECT_EQ(this->GetParam(), mers[0].to_str());
  EXPECT_EQ(inserted, mers[1]);
  EXPECT_EQ(this->GetParam(), mers[2].to_str());
}

TYPED_TEST(MerDNA, OperatorShift) {
  typename TypeParam::Type m(this->GetParam());
  std::ostringstream os;
  os << m;
  EXPECT_EQ(this->GetParam(), os.str());
}

TYPED_TEST(MerDNA, GetBits) {
  typename TypeParam::Type m(this->GetParam());
  std::uniform_int_distribution<long int> start_position(0, this->GetParam().size() - 2);
  std::uniform_int_distribution<long int> bit_length;
  for(unsigned int i = 0; i < 20; ++i) {
    long int start   = start_position(prg);
    long int max_len =
      std::min((long int)(this->GetParam().size() - start),
               (long int)(8 * sizeof(typename TypeParam::Type::base_type)));
    const auto length_range = decltype(bit_length)::param_type(1, max_len - 1);
    long int len = bit_length(prg, length_range);

    // Get bits by right-shifting
    typename TypeParam::Type cm(m);
    for(unsigned int j = 1; j < start; j += 2)
      cm.shift_right(0); // Shift by 2 bits
    typename TypeParam::Type::base_type y = cm.word(0);
    if(start & 0x1)
      y >>= 1;
    y &= ((typename TypeParam::Type::base_type)1 << len) - 1;

    EXPECT_EQ(y, m.get_bits(start, len));
  }
}
TYPED_TEST(MerDNA, GetBases) {
  typename TypeParam::Type m(this->GetParam());

  for(std::string::const_reverse_iterator it = this->GetParam().rbegin(); it != this->GetParam().rend(); ++it)
    EXPECT_EQ(*it, (char)m.base(it - this->GetParam().rbegin()));

  const char bases[4] = { 'A', 'C', 'G', 'T' };
  for(const char* it = bases; it != bases + 4; ++it) {
    typename TypeParam::Type n(m);
    for(size_t j = 0; j < this->GetParam().size(); ++j)
      n.base(j) = *it;
    typename TypeParam::Type m_expected(std::string(this->GetParam().size(), *it));
    EXPECT_EQ(m_expected, n);
  }
}

char rc_base(char c) {
  switch(c) {
  case 'A': case 'a': return 'T';
  case 'C': case 'c': return 'G';
  case 'G': case 'g': return 'C';
  case 'T': case 't': return 'A';
  }
  return 'A'; // Should never be reached
}

TYPED_TEST(MerDNA, ReverseComplement) {
  typename TypeParam::Type m(this->GetParam());
  std::string rc(this->GetParam().size(), 'A');
  for(size_t i = 0; i < rc.size(); ++i)
    rc[i] = rc_base(this->GetParam()[this->GetParam().size() - 1 - i]);
  EXPECT_EQ(this->GetParam().size(), m.k());
  m.reverse_complement();
  EXPECT_EQ(rc, m.to_str());
  typename TypeParam::Type rm(rc);
  EXPECT_EQ(rm, m);
  EXPECT_EQ(m, m.get_reverse_complement().get_reverse_complement());
}

TYPED_TEST(MerDNA, Canonical) {
  typename TypeParam::Type m(this->GetParam());
  typename TypeParam::Type canonical = m.get_canonical();

  EXPECT_FALSE(m < canonical);
  EXPECT_TRUE(canonical <= m);
  EXPECT_TRUE(canonical == m || canonical == m.get_reverse_complement());
  m.canonicalize();
  EXPECT_EQ(canonical, m.get_canonical());
}

template<typename T>
class MerStorage : public ::testing::Test { };
typedef ::testing::Types<uint32_t, uint64_t
#ifdef HAVE_INT128
                         , unsigned __int128
#endif
                         > MerStorageWords;
TYPED_TEST_CASE(MerStorage, MerStorageWords);

TYPED_TEST(MerStorage, InlineOwnership) {
  typedef mer_dna_ns::mer_base_stack<TypeParam, 2> Storage;
  typedef mer_dna_ns::mer_base_static<TypeParam, 100, Storage> Mer;
  static_assert(std::is_empty<mer_dna_ns::mer_base<Mer>>::value,
                "mer_base must not store a data pointer");
  static_assert(sizeof(Mer) == 2 * sizeof(TypeParam),
                "Static stack mers must contain only inline words");
  Mer::k(4 * sizeof(TypeParam) + 1);
  Mer m;
  const uintptr_t object = reinterpret_cast<uintptr_t>(&m);
  const uintptr_t data = reinterpret_cast<uintptr_t>(m.data());
  EXPECT_GE(data, object);
  EXPECT_LE(data + m.nb_bytes(), object + sizeof(m));
  EXPECT_EQ(m.data(), m.data__());
}

TYPED_TEST(MerStorage, DynamicCapacity) {
  typedef mer_dna_ns::mer_base_stack<TypeParam, 2> Storage;
  typedef mer_dna_ns::mer_base_dynamic<TypeParam, Storage> Mer;
  const unsigned int capacity = 8 * sizeof(TypeParam);
  Mer m(capacity);
  EXPECT_EQ(2U, m.nb_words());
  EXPECT_EQ(std::string(capacity, 'A'), m.to_str());
  m.polyT();
  EXPECT_EQ(std::string(capacity, 'T'), m.to_str());
  Mer shorter(capacity - 1);
  EXPECT_EQ(std::string(capacity - 1, 'A'), shorter.to_str());
  EXPECT_THROW((Mer(0U)), std::length_error);
  EXPECT_THROW((Mer(capacity + 1)), std::length_error);
  EXPECT_THROW((Mer(std::string(capacity + 1, 'A'))), std::length_error);
  const std::string oversized(capacity + 1, 'A');
  EXPECT_THROW((Mer(oversized.c_str())), std::length_error);
  EXPECT_THROW(Mer::k(capacity + 1), std::length_error);
}

TYPED_TEST(MerStorage, StaticCapacityAndIndependentLengths) {
  typedef mer_dna_ns::mer_base_stack<TypeParam, 2> Storage;
  typedef mer_dna_ns::mer_base_static<TypeParam, 101, Storage> Stack;
  typedef mer_dna_ns::mer_base_static<TypeParam, 101> Heap;
  typedef mer_dna_ns::mer_base_static<TypeParam, 102, Storage> Other;
  const unsigned int capacity = 8 * sizeof(TypeParam);
  EXPECT_EQ(22U, Stack::k());
  Heap::k(capacity + 1);
  Other::k(1);
  EXPECT_EQ(22U, Stack::k(capacity));
  EXPECT_THROW(Stack::k(0), std::length_error);
  EXPECT_THROW(Stack::k(capacity + 1), std::length_error);
  EXPECT_EQ(capacity, Stack::k());
  EXPECT_EQ(capacity + 1, Heap::k());
  EXPECT_EQ(1U, Other::k());
  Stack m;
  m.polyT();
  EXPECT_EQ(std::string(capacity, 'T'), m.to_str());
  EXPECT_THROW((Stack(capacity - 1)), mer_dna_ns::different_k_error);
}

template<typename Mer>
void check_small_inline_operations(unsigned int capacity) {
  for(unsigned int k = 1; k <= capacity; ++k) {
    SCOPED_TRACE(::testing::Message() << "k:" << k);
    Mer::k(k);
    std::string sequence(k, 'A');
    for(unsigned int i = 0; i < k; ++i)
      sequence[i] = "ACGT"[i % 4];
    Mer m(k, sequence.c_str());
    const Mer& constant = m;
    EXPECT_EQ(sequence.front(), static_cast<char>(constant.base(k - 1)));
    EXPECT_EQ(sequence, m.to_str());
    EXPECT_EQ(sequence.front(), m.shift_left('T'));
    sequence = sequence.substr(1) + 'T';
    EXPECT_EQ(sequence, m.to_str());
    EXPECT_EQ(sequence.back(), m.shift_right('C'));
    sequence = 'C' + sequence.substr(0, k - 1);
    EXPECT_EQ(sequence, m.to_str());
    EXPECT_EQ(sequence.find_first_not_of(sequence[0]) == std::string::npos,
              m.is_homopolymer());
    std::string reverse(sequence.rbegin(), sequence.rend());
    for(char& base : reverse)
      base = rc_base(base);
    EXPECT_EQ(reverse, m.get_reverse_complement().to_str());
    EXPECT_EQ(std::min(sequence, reverse), m.get_canonical().to_str());
    m.canonicalize();
    EXPECT_EQ(std::min(sequence, reverse), m.to_str());
    Mer copy(m);
    EXPECT_EQ(m, copy);
    EXPECT_FALSE(m < copy);
    copy.polyT();
    EXPECT_TRUE(copy.is_homopolymer());
    EXPECT_TRUE(m <= copy);
  }
}

TYPED_TEST(MerStorage, SmallInlineOperations) {
  typedef mer_dna_ns::mer_base_stack<TypeParam, 1> One;
  typedef mer_dna_ns::mer_base_stack<TypeParam, 2> Two;
  check_small_inline_operations<mer_dna_ns::mer_base_dynamic<TypeParam, One>>(
      4 * sizeof(TypeParam));
  check_small_inline_operations<mer_dna_ns::mer_base_static<TypeParam, 104, One>>(
      4 * sizeof(TypeParam));
  check_small_inline_operations<mer_dna_ns::mer_base_dynamic<TypeParam, Two>>(
      8 * sizeof(TypeParam));
  check_small_inline_operations<mer_dna_ns::mer_base_static<TypeParam, 105, Two>>(
      8 * sizeof(TypeParam));
}

TEST(MerStorageDefaults, InsufficientDefaultCapacity) {
  typedef mer_dna_ns::mer_base_static<uint32_t, 103,
                                    mer_dna_ns::mer_base_stack<uint32_t, 1>> Mer;
  EXPECT_EQ(22U, Mer::k());
  EXPECT_THROW((Mer()), std::length_error);
  EXPECT_EQ(22U, Mer::k(16));
  Mer m;
  EXPECT_EQ(std::string(16, 'A'), m.to_str());
}

template<typename Mer>
void check_trivial_value_type() {
  static_assert(std::is_trivially_copyable<Mer>::value, "Mer must be trivially copyable");
  static_assert(std::is_trivially_copy_constructible<Mer>::value, "Copy construction must be trivial");
  static_assert(std::is_trivially_copy_assignable<Mer>::value, "Copy assignment must be trivial");
  static_assert(std::is_trivially_move_constructible<Mer>::value, "Move construction must be trivial");
  static_assert(std::is_trivially_move_assignable<Mer>::value, "Move assignment must be trivial");
  static_assert(std::is_trivially_destructible<Mer>::value, "Destruction must be trivial");
  static_assert(std::is_nothrow_copy_constructible<Mer>::value, "Copy construction must not throw");
  static_assert(std::is_nothrow_move_constructible<Mer>::value, "Move construction must not throw");
  static_assert(std::is_nothrow_copy_assignable<Mer>::value, "Copy assignment must not throw");
  static_assert(std::is_nothrow_move_assignable<Mer>::value, "Move assignment must not throw");
  static_assert(std::is_nothrow_destructible<Mer>::value, "Destruction must not throw");
}

TYPED_TEST(MerStorage, TrivialInlineValues) {
  typedef mer_dna_ns::mer_base_stack<TypeParam, 1> One;
  typedef mer_dna_ns::mer_base_stack<TypeParam, 2> Two;
  typedef mer_dna_ns::mer_base_static<TypeParam, 106, One> StaticOne;
  typedef mer_dna_ns::mer_base_static<TypeParam, 106, Two> StaticTwo;
  check_trivial_value_type<StaticOne>();
  check_trivial_value_type<StaticTwo>();
  check_trivial_value_type<mer_dna_ns::mer_base_dynamic<TypeParam, One>>();
  check_trivial_value_type<mer_dna_ns::mer_base_dynamic<TypeParam, Two>>();
  static_assert(sizeof(StaticOne) == sizeof(TypeParam), "One-word mer must have no size overhead");
  static_assert(sizeof(StaticTwo) == 2 * sizeof(TypeParam), "Inline mer must have no size overhead");
  static_assert(alignof(StaticOne) == alignof(TypeParam), "One-word mer must keep word alignment");
  static_assert(alignof(StaticTwo) == alignof(TypeParam), "Inline mer must keep word alignment");
  static_assert(std::is_standard_layout<StaticOne>::value, "Static inline mer must have standard layout");
}

TYPED_TEST(MerStorage, NothrowHeapMoves) {
  typedef mer_dna_ns::mer_base_static<TypeParam> Static;
  typedef mer_dna_ns::mer_base_dynamic<TypeParam> Dynamic;
  static_assert(std::is_nothrow_move_constructible<Static>::value, "Static heap moves must not throw");
  static_assert(std::is_nothrow_move_assignable<Static>::value, "Static heap move assignment must not throw");
  static_assert(std::is_nothrow_move_constructible<Dynamic>::value, "Dynamic heap moves must not throw");
  static_assert(std::is_nothrow_move_assignable<Dynamic>::value, "Dynamic heap move assignment must not throw");
}

template<typename Storage>
void check_dynamic_assignment() {
  typedef typename Storage::base_type Word;
  typedef mer_dna_ns::mer_base_dynamic<Word, Storage> Mer;
  Mer source("ACGTACGT");
  Mer destination("TTTTTTTT");
  destination = source;
  EXPECT_NE(destination.data(), source.data());
  EXPECT_EQ(source, destination);
  source.polyG();
  EXPECT_EQ("ACGTACGT", destination.to_str());
  destination = destination;
  EXPECT_EQ("ACGTACGT", destination.to_str());
  Mer different_length("ACGT");
  destination = different_length;
  EXPECT_EQ(4U, destination.k());
  EXPECT_EQ("ACGT", destination.to_str());
  EXPECT_EQ("ACGT", different_length.to_str());
  const unsigned int long_length = 8 * sizeof(Word) + 1;
  Mer longer(std::string(long_length, 'G'));
  destination = longer;
  EXPECT_EQ(long_length, destination.k());
  EXPECT_EQ(longer, destination);
  EXPECT_NE(longer.data(), destination.data());
  destination = std::move(different_length);
  EXPECT_EQ(4U, destination.k());
  EXPECT_EQ("ACGT", destination.to_str());
  const unsigned int middle_length = 4 * sizeof(Word) + 1;
  destination = Mer(std::string(middle_length, 'C'));
  EXPECT_EQ(middle_length, destination.k());
  EXPECT_EQ(std::string(middle_length, 'C'), destination.to_str());
}

TYPED_TEST(MerStorage, DynamicAssignment) {
  check_dynamic_assignment<mer_dna_ns::mer_base_heap<TypeParam>>();
  check_dynamic_assignment<mer_dna_ns::mer_base_stack<TypeParam, 3>>();
}

} // namespace {
