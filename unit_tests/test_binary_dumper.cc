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
typedef hash_counter::array::eager_iterator iterator;

TEST(BinaryDumper, ReadWrite) {
  static const int mer_len = 50;
  static const int hash_size = 5000;
  static const int nb = hash_size;
  static const int hash_val_len = 5;
  static const int dump_counter_len = 1;
  static const char* file = "./binary_dumper";
  file_unlink f(file);

  mer_dna::k(mer_len);
  hash_counter hash(hash_size, mer_len * 2, 5 /* val len */, 1 /* nb threads */);

  mer_dna m;
  for(int i = 0; i < nb; i++) {
    m.randomize();
    const uint64_t rval = random_bits(9);
    hash.add(m, rval);
    uint64_t val;
    ASSERT_TRUE(hash.ary()->get_val_for_key(m, &val));
    EXPECT_EQ(rval, val);
  }

  // Dump without zeroing to check dumped content against in memory hash
  {
    file_header h;
    h.fill_standard();
    h.update_from_ary(*hash.ary());
    dumper d(dump_counter_len, 4, file, &h);
    d.one_file(true);
    d.zero_array(false);
    d.dump(hash.ary());
  }

  // Check dumped content
  {
    file_header h;
    std::ifstream is(file);
    h.read(is);
    EXPECT_STREQ(dumper::format, h.format().c_str());
    EXPECT_EQ(dump_counter_len, h.counter_len());

    reader r(is, dump_counter_len);

    const uint64_t max_val = ((uint64_t)1 << (8 * dump_counter_len)) - 1;
    int count = 0;
    while(r.next()) {
      uint64_t val = 0;
      bool present = hash.ary()->get_val_for_key(r.key(), &val);
      EXPECT_TRUE(present);
      if(present) {
        EXPECT_EQ(std::min(max_val, val), r.val());
        ++count;
      }
    }
    EXPECT_EQ(nb, count);
  }

  // Dump with zeroing and check hash is empty
  {
    file_header h;
    h.fill_standard();
    h.update_from_ary(*hash.ary());
    dumper d(dump_counter_len, 4, file, &h);
    d.one_file(true);
    d.dump(hash.ary());
  }
  {
    iterator it = hash.ary()->eager_slice(0, 1);
    uint64_t count = 0;
    while(it.next()) ++count;
    EXPECT_EQ((uint64_t)0, count);
  }
}

} // namespace {
