#include <string>
#include <fstream>

#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>
#include <jellyfish/parser_common.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>

namespace {
using std::string;
using std::istringstream;
template<typename Iterator>
struct opened_streams {
  Iterator begin_, end_;

  opened_streams(Iterator begin, Iterator end) : begin_(begin), end_(end) { }
  jellyfish::stream_type next() {
    jellyfish::stream_type res;
    if(begin_ != end_) {
      res.standard.reset(*begin_);
      ++begin_;
    }
    return res;
  }
};
typedef jellyfish::mer_overlap_sequence_parser<opened_streams<std::ifstream**> > parser_type;

static const char* line_endings[2] = {"\n", "\r\n"};


TEST(MerOverlapSequenceParser, OneSmallSequence) {
  static const char* seq = "ATTACCTTGTACCTTCAGAGC";
  const char* file_name = "OneSmallSequence.fa";

  for(auto le : line_endings) {
    file_unlink fu(file_name);
    { std::ofstream sequence(file_name);
      sequence << ">header" << le << seq;
    }
    auto sequence = new std::ifstream(file_name);
    opened_streams<std::ifstream**> streams(&sequence, &sequence + 1);

    parser_type parser(10, 1, 10, 100, streams);

    parser_type::job j(parser);
    ASSERT_FALSE(j.is_empty());
    EXPECT_EQ(strlen(seq), (size_t)(j->end - j->start));
    EXPECT_STREQ(seq, j->start);
    j.release();

    parser_type::job j2(parser);
    EXPECT_TRUE(j2.is_empty());

    EXPECT_EQ((size_t)1, parser.nb_files());
    EXPECT_EQ((size_t)1, parser.nb_reads());
  }
}

string generate_sequences(std::ostream& os, int a, int b, int nb, const char* le, bool fastq = false) {
  static const char bases[4] = {'A', 'C', 'G', 'T' };
  string res;
  for(int i = 0; i < nb; ++i) {
    int len = a + random() % b;
    os << (fastq ? "@" : ">") << "r" << i << " " << len << le;
    for(int j = 0; j < len; ++j) {
      char b = bases[random() & 0x3];
      os << b;
      res += b;
      if(random() % 16 == 0)
        os << le;
    }
    if(i != nb - 1)
      res += 'N';
    os << le;
    if(fastq) {
      os << '+' << le;
      for(int j = 0; j < len; ++j) {
        if(random() % 16 == 0)
          os << le;
        os << "#";
      }
      os << le;
    }
  }
  return res;
}

TEST(MerOverlapSequenceParser, ManySmallSequences) {
  const char* file_name = "ManySmallSequences.fa";
  static const int nb_reads = 20;
  static const int mer_len = 10;

  for(auto le : line_endings) {
    file_unlink fu(file_name);
    std::ofstream o_sequence(file_name);
    string res = generate_sequences(o_sequence, 20, 64, nb_reads, le);
    o_sequence.close();

    auto sequence = new std::ifstream(file_name);
    opened_streams<std::ifstream**> streams(&sequence, &sequence + 1);

    parser_type parser(mer_len, 1, 10, 100, streams);
    size_t offset = 0;
    while(true) {
      parser_type::job j(parser);
      if(j.is_empty())
        break;
      SCOPED_TRACE(::testing::Message() << offset << ": " << res.substr(offset, j->end - j->start));
      EXPECT_EQ(res.substr(offset, j->end - j->start).c_str(), string(j->start, j->end - j->start));
      offset += j->end - j->start - (mer_len - 1);
    }
    EXPECT_EQ(res.size(), offset + mer_len - 1);

    EXPECT_EQ((size_t)1, parser.nb_files());
    EXPECT_EQ((size_t)nb_reads, parser.nb_reads());
  }
}

TEST(MerOverlapSequenceParser, BigSequences) {
  const char* file_name1 = "BigSequences1.fa";
  const char* file_name2 = "BigSequences2.fa";

  for(auto le : line_endings) {
    file_unlink fu1(file_name1);
    file_unlink fu2(file_name2);

    std::ofstream o_sequence(file_name1);
    const string res0 = generate_sequences(o_sequence, 200, 100, 3, le);
    o_sequence.close();
    o_sequence.open(file_name2);
    const string res1 = generate_sequences(o_sequence, 200, 100, 3, le);
    o_sequence.close();

    std::ifstream* tmps[2];
    tmps[0] = new std::ifstream(file_name1);
    tmps[1] = new std::ifstream(file_name2);
    opened_streams<std::ifstream**> streams(tmps, tmps + 2);

    static const int mer_len = 10;
    parser_type parser(mer_len, 1, 10, 100, streams);

    size_t offset = 0;
    while(offset < res0.size() - mer_len + 1) {
      parser_type::job j(parser);
      ASSERT_FALSE(j.is_empty());
      SCOPED_TRACE(::testing::Message() << offset << ": " << res0.substr(offset, j->end - j->start));
      EXPECT_EQ(res0.substr(offset, j->end - j->start).c_str(), string(j->start, j->end - j->start));
      offset += j->end - j->start - (mer_len - 1);
    }
    EXPECT_EQ(res0.size(), offset + mer_len - 1);

    offset = 0;
    while(offset < res1.size() - mer_len + 1) {
      parser_type::job j(parser);
      ASSERT_FALSE(j.is_empty());
      SCOPED_TRACE(::testing::Message() << offset << ": " << res1.substr(offset, j->end - j->start));
      EXPECT_EQ(res1.substr(offset, j->end - j->start).c_str(), string(j->start, j->end - j->start));
      offset += j->end - j->start - (mer_len - 1);
    }
    EXPECT_EQ(res1.size(), offset + mer_len - 1);

    parser_type::job j2(parser);
    EXPECT_TRUE(j2.is_empty());

    EXPECT_EQ((size_t)2, parser.nb_files());
    EXPECT_EQ((size_t)6, parser.nb_reads());
  }
}

TEST(MerOverlapSequenceParser, Fastq) {
  const char* file_name = "Fastq.fa";
  static const int nb_reads = 100;
  static const int mer_len = 20;

  for(auto le : line_endings) {
    file_unlink fu(file_name);
    std::ofstream o_sequence(file_name);
    string res = generate_sequences(o_sequence, 10, 50, nb_reads, le, true);
    o_sequence.close();

    auto sequence = new std::ifstream(file_name);
    opened_streams<std::ifstream**> streams(&sequence, &sequence + 1);
    parser_type parser(mer_len, 1, 10, 100, streams);

    size_t offset = 0;
    while(true) {
      parser_type::job j(parser);
      if(j.is_empty())
        break;
      //    SCOPED_TRACE(::testing::Message() << offset << ": " << res.substr(offset, j->end - j->start));
      EXPECT_EQ(res.substr(offset, j->end - j->start).c_str(), string(j->start, j->end - j->start));
      offset += j->end - j->start - (mer_len - 1);
    }
    EXPECT_EQ(res.size(), offset + mer_len - 1);

    EXPECT_EQ((size_t)1, parser.nb_files());
    EXPECT_EQ((size_t)nb_reads, parser.nb_reads());
  }
}
} // namespace {
