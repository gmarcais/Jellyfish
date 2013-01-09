#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/file_header.hpp>
#include <jellyfish/binary_dumper.hpp>

namespace {
using jellyfish::mer_dna;
using jellyfish::file_header;
typedef jellyfish::cooperative::hash_counter<mer_dna> hash_counter;
typedef jellyfish::binary_dumper<hash_counter::array> dumper;
typedef jellyfish::binary_reader<mer_dna, uint64_t> reader;

TEST(BinaryDumper, ReadWrite) {
  static const int mer_len = 50;
  static const int nb = 5000;
  static const int hash_val_len = 5;
  static const int dump_counter_len = 1;
  static const char* file = "./binary_dumper";
  //  file_unlink f(file);

  mer_dna::k(mer_len);
  hash_counter hash(nb, mer_len * 2, 5 /* val len */, 1 /* nb threads */);

  mer_dna m;
  for(int i = 0; i < nb; i++) {
    m.randomize();
    hash.add(m, random_bits(9));
  }

  {
    file_header h;
    h.fill_standard();
    h.update_from_ary(*hash.ary());
    dumper d(dump_counter_len, 4, file, hash.ary(), &h);
    d.one_file(true);
    d.dump();
  }

  file_header h;
  std::ifstream is(file);
  h.read(is);
  EXPECT_STREQ(dumper::format, h.format().c_str());
  EXPECT_EQ(dump_counter_len, h.counter_len());

  reader r(is, dump_counter_len);

  int count = 0;
  while(r.next()) {
    uint64_t val = 0;
    EXPECT_TRUE(hash.ary()->get_val_for_key(r.key(), &val));
    EXPECT_EQ(val, r.val());
    ++count;
  }
  EXPECT_EQ(nb, count);
}

} // namespace {
