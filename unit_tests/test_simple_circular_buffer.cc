#include <gtest/gtest.h>
#include <jellyfish/simple_circular_buffer.hpp>

namespace {
  // template<typename T>
  // class CircularBufferTest : public ::testing::Test { };

  // typedef ::testing::Types<circular_buffer::fixed<int, 16>,
  //                          circular_buffer::dyn<int> > CircularBufferTypes;
  // TYPED_TEST_CASE(CircularBufferTest, CircularBufferTypes);
  static const int capa = 16;

  class test_simple_circular_buffer : 
    public jellyfish::simple_circular_buffer::pre_alloc<int, capa> {
  public:
    typedef jellyfish::simple_circular_buffer::pre_alloc<int, capa> super;
    test_simple_circular_buffer(int* data) : super(data) { }

    int next_index(int i) const { return super::next_index(i); }
    int prev_index(int i) const { return super::prev_index(i); }
    int front_ptr() const { return front_; }
    int back_ptr() const { return back_; }
  };

  TEST(SimpleCircularBufferTest, FillEmpty) {
    int ary[capa];
    test_simple_circular_buffer buffer(ary);
    EXPECT_EQ(capa, buffer.capacity());

    EXPECT_TRUE(buffer.empty());
    for(int i = 0; i < buffer.capacity(); ++i) {
      SCOPED_TRACE(::testing::Message() << "i=" << i);
      EXPECT_FALSE(buffer.full());
      EXPECT_EQ(i, buffer.size());
      buffer.push_back(i);
      EXPECT_EQ(i, buffer.back());
      EXPECT_FALSE(buffer.empty());
    }
    EXPECT_TRUE(buffer.full());

    for(int i = 0; i < buffer.capacity(); ++i) {
      SCOPED_TRACE(::testing::Message() << "i=" << i);
      EXPECT_FALSE(buffer.empty());
      EXPECT_EQ(i, buffer.front());
      EXPECT_EQ(buffer.capacity() - i, buffer.size());
      buffer.pop_front();
      EXPECT_FALSE(buffer.full());
    }
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(0, buffer.size());
  }

  TEST(SimpleCircularBufferTest, Usefull) {
    int ary[capa];
    test_simple_circular_buffer buffer(ary);
    EXPECT_EQ(capa, buffer.capacity());

    int i = 0;
    for( ; i < buffer.capacity(); ++i) {
      EXPECT_TRUE(buffer.push_back());
      buffer.back() = i;
    }

    for( ; i < 10 * buffer.capacity(); ++i) {
      EXPECT_EQ(i - capa, buffer.front());
      buffer.pop_front();
      EXPECT_TRUE(buffer.push_back());
      buffer.back() = i;
    }
  }
}
