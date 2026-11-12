/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#include <memory>

#include <gtest/gtest.h>
#include <unit_tests/test_main.hpp>
#include <jellyfish/atomic_bits_array.hpp>

namespace {
TEST(AtomicBitsArray, Fill) {
  static const size_t size = 2066;
  static const int    bits = 3;
  jellyfish::atomic_bits_array<unsigned char> ary(bits, size);
  std::unique_ptr<unsigned char[]> data(new unsigned char[size]);

  for(size_t i = 0; i < size; ++i) {
    data[i] = random_bits(bits);
    auto e = ary[i];
    e.get();
    EXPECT_TRUE(e.set(data[i]));
  }

  auto it = ary.begin();
  auto it_end = ary.end();
  for(size_t i = 0; i < size; ++i, ++it) {
    auto e = ary[i];
    EXPECT_EQ(data[i], e.get());
    EXPECT_EQ(data[i], (unsigned char)e);
    ASSERT_NE(it_end, it);
    EXPECT_EQ(data[i], *it);
  }
  EXPECT_EQ(it_end, it);
}
}
