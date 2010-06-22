#include <gtest/gtest.h>
#include <misc.hpp>
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
    ASSERT_EQ(i, floorLog2(this->x << i));
    ASSERT_EQ(i, floorLog2((this->x << i) | ((this->x << i) - 1)));
  }
}
