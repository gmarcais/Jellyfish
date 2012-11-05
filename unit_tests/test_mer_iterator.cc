#include <gtest/gtest.h>
#include <jflib/tmpstream.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/mer_iterator.hpp>

namespace {
using std::string;
using jellyfish::mer_dna;
typedef std::vector<string> string_vector;
typedef jellyfish::mer_overlap_sequence_parser<jflib::tmpstream*> parser_type;
typedef jellyfish::mer_iterator<parser_type, mer_dna> mer_iterator_type;

string generate_sequence(int len) {
  static const char bases[4] = { 'A', 'C', 'G', 'T' };
  string res;

  for(int i = 0; i < len; ++i)
    res += bases[random() % 0x3];

  return res;
}

TEST(MerIterator, Sequence) {
  string_vector sequences;
  jflib::tmpstream input_fasta;

  static const int nb_sequences = 100;
  mer_dna::k(35);
  for(int i = 0; i < nb_sequences; ++i) {
    sequences.push_back(generate_sequence(20 + random() % 200));
    input_fasta << ">" << i << "\n" << sequences.back() << "\n";
  }
  input_fasta.seekg(0);

  parser_type parser(mer_dna::k(), 10, 100, &input_fasta, &input_fasta + 1);
  mer_iterator_type mit(parser);
  for(string_vector::const_iterator it = sequences.begin(); it != sequences.end(); ++it) {
    //    std::cerr << (it - sequences.begin()) << ":" << it->size() << ":" << *it << "\n";
    if(it->size() >= mer_dna::k()) {
      for(int i = 0; i < it->size() - (mer_dna::k() - 1); ++i, ++mit) {
        //        std::cerr << "i:" << i << " " << "\n";
        ASSERT_NE(mer_iterator_type(), mit);
        EXPECT_EQ(it->substr(i, mer_dna::k()), mit->to_str());
      }
    }
  }
  EXPECT_EQ(mer_iterator_type(), mit);
}
} // namespace {
