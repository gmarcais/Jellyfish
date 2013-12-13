#include <gtest/gtest.h>
#include <jflib/tmpstream.hpp>
#include <jellyfish/whole_sequence_parser.hpp>
#include <unit_tests/test_main.hpp>

namespace {
using std::string;
using std::istringstream;
template<typename Iterator>
struct opened_streams {
  Iterator begin_, end_;

  opened_streams(Iterator begin, Iterator end) : begin_(begin), end_(end) { }
  std::unique_ptr<std::istream> next() {
    std::unique_ptr<std::istream> res;
    if(begin_ != end_) {
      res.reset(*begin_);
      ++begin_;
    }
    return res;
  }
};
typedef jellyfish::whole_sequence_parser<opened_streams<jflib::tmpstream**> > parser_type;

TEST(SequenceParser, Fasta) {
  static const char* seq1 = "ATTACCTTGTACCTTCAGAGC";
  static const char* seq2 = "TTCGATCCCTTGATAATTAGTCACGTTAGCT";
  jflib::tmpstream* sequence = new jflib::tmpstream;
  *sequence << ">header1\n"
            << seq1 << "\n\n"
            << ">header2\n"
            << seq2 << "\n\n\n"
            << seq1;
  sequence->seekg(0);

  opened_streams<jflib::tmpstream**> streams(&sequence, &sequence + 1);
  parser_type parser(10, 1, 1, streams);

  {
    parser_type::job j(parser);
    ASSERT_FALSE(j.is_empty());
    ASSERT_EQ((size_t)1, j->nb_filled);
    EXPECT_STREQ("header1", j->data[0].header.c_str());
    EXPECT_STREQ(seq1, j->data[0].seq.c_str());
  }

  {
    parser_type::job j(parser);
    ASSERT_FALSE(j.is_empty());
    ASSERT_EQ((size_t)1, j->nb_filled);
    EXPECT_STREQ("header2", j->data[0].header.c_str());
    EXPECT_EQ(strlen(seq2) + strlen(seq1), j->data[0].seq.size());
    EXPECT_STREQ(seq2, j->data[0].seq.substr(0, strlen(seq2)).c_str());
    EXPECT_STREQ(seq1, j->data[0].seq.substr(strlen(seq2)).c_str());
  }

  {
    parser_type::job j(parser);
    EXPECT_TRUE(j.is_empty());
  }
}

TEST(SequenceParser, Fastq) {
  static const char* seq1 = "ATTACCTTGTACCTTCAGAGC";
  static const char* seq2 = "TTCGATCCCTTGATAATTAGTCACGTTAGCT";
  jflib::tmpstream* sequence = new jflib::tmpstream;
  *sequence << "@header1\n"
           << seq1 << "\n\n"
           << "+and some stuff\n"
           << seq1 << "\n"
           << "@header2\n"
           << seq2 << "\n\n\n"
           << seq1 << "\n"
           << "+\n"
           << seq1 << "\n"
           << seq2 << "\n";
  sequence->seekg(0);

  opened_streams<jflib::tmpstream**> streams(&sequence, &sequence + 1);
  parser_type parser(10, 1, 1, streams);

  {
    parser_type::job j(parser);
    ASSERT_FALSE(j.is_empty());
    ASSERT_EQ((size_t)1, j->nb_filled);
    EXPECT_STREQ("header1", j->data[0].header.c_str());
    EXPECT_STREQ(seq1, j->data[0].seq.c_str());
    EXPECT_STREQ(seq1, j->data[0].qual.c_str());
  }

  {
    parser_type::job j(parser);
    ASSERT_FALSE(j.is_empty());
    ASSERT_EQ((size_t)1, j->nb_filled);
    EXPECT_STREQ("header2", j->data[0].header.c_str());
    EXPECT_EQ(strlen(seq2) + strlen(seq1), j->data[0].seq.size());
    EXPECT_EQ(strlen(seq2) + strlen(seq1), j->data[0].qual.size());
    EXPECT_STREQ(seq2, j->data[0].seq.substr(0, strlen(seq2)).c_str());
    EXPECT_STREQ(seq1, j->data[0].seq.substr(strlen(seq2)).c_str());
    EXPECT_STREQ(seq1, j->data[0].qual.substr(0, strlen(seq1)).c_str());
    EXPECT_STREQ(seq2, j->data[0].qual.substr(strlen(seq1)).c_str());
  }

  {
    parser_type::job j(parser);
    EXPECT_TRUE(j.is_empty());
  }
}

TEST(SequenceParser, FastaMany) {
  auto sequence = new jflib::tmpstream;
  static const int nb_sequences = 1000;
  ASSERT_TRUE(sequence->good());
  for(int i = 0; i < nb_sequences; ++i) {
    const int seq_len = random_bits(8) + 10;
    *sequence << ">" << i << " " << seq_len << "\n";
    for(int j = 0; j < seq_len; ++j) {
      *sequence << (char)('A' + j % 26);
      if(random_bits(6) == 0)
        *sequence << '\n';
    }
    *sequence << '\n';
  }
  sequence->seekg(0);

  opened_streams<jflib::tmpstream**> streams(&sequence, &sequence + 1);
  parser_type parser(10, 13, 1, streams);
  int got_sequences = 0;
  while(true) {
    parser_type::job job(parser);
    if(job.is_empty())
      break;
    for(size_t i = 0; i < job->nb_filled; ++i) {
      ++got_sequences;
      const size_t len_pos = job->data[i].header.find_first_of(" ");
      const int seq_len = atoi(job->data[i].header.c_str() + len_pos + 1);
      ASSERT_LE(10, seq_len);
      for(int j = 0; j < seq_len; ++j)
        EXPECT_EQ('A' + (j % 26), job->data[i].seq[j]);
    }
  }
  EXPECT_EQ(nb_sequences, got_sequences);
}

}
