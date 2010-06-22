#include <gtest/gtest.h>
#include <allocators_malloc.hpp>
#include <allocators_mmap.hpp>
#include <allocators_shm.hpp>

using testing::Types;

#define SMALL_SIZE      ((size_t)4096)
template <class large_block>
class LargeBlockTest : public ::testing::Test
{
public:
  large_block   block;

  LargeBlockTest() : block(SMALL_SIZE) {}
  void *double_size() {
    return block.realloc(block.get_size() * 2);
  }
};

typedef Types<allocators::malloc, allocators::mmap, allocators::shm> Implementations;

TYPED_TEST_CASE(LargeBlockTest, Implementations);

TYPED_TEST(LargeBlockTest, SuccessfulAllocation) {
  EXPECT_NE((void *)0, this->block.get_ptr());
  EXPECT_EQ(SMALL_SIZE, this->block.get_size());
  void *new_ptr = this->double_size();
  EXPECT_NE((void *)0, new_ptr);
  EXPECT_NE((void *)0, this->block.get_ptr());
  EXPECT_EQ(new_ptr, this->block.get_ptr());
  EXPECT_EQ(SMALL_SIZE * 2, this->block.get_size());
}
