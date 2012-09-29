#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>
#include <jellyfish/misc.hpp>
#include <stdint.h>

using testing::Types;

template <typename T>
class FloorLog2Test : public ::testing::Test {
public:
  T x;
  
  FloorLog2Test() : x(1) {}
};

typedef Types<uint32_t, uint64_t> Implementations;
TYPED_TEST_CASE(FloorLog2Test, Implementations);

TYPED_TEST(FloorLog2Test, FloorLog2) {
  unsigned int i = 0;

  for(i = 0; i < 8 * sizeof(this->x); i++) {
    ASSERT_EQ(i, jellyfish::floorLog2(this->x << i));
    ASSERT_EQ(i, jellyfish::floorLog2((this->x << i) | ((this->x << i) - 1)));
  }
}



TEST(Random, Bits) {
  uint64_t m = 0;
  uint64_t m2 = 0;
  int not_zero = 0;
  for(int i = 0; i < 1024; ++i) {
    m        += jellyfish::random_bits(41);
    m2       += random_bits(41);
    not_zero += random_bits() > 0;
  }
  EXPECT_LT((uint64_t)1 << 49, m); // Should be false with very low probability
  EXPECT_LT((uint64_t)1 << 49, m2); // Should be false with very low probability
  EXPECT_LT(512, not_zero);
}
