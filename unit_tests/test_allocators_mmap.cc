/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#include <gtest/gtest.h>
#include <jellyfish/allocators_mmap.hpp>

namespace {
TEST(AllocMmap, Simple) {
  static const size_t size = 4096;
  allocators::mmap mem(size);

  EXPECT_EQ(size, mem.get_size());
  EXPECT_NE((void*)0, mem.get_ptr());
  //  char* ptr = (char*)mem.get_ptr();
  // char c = ptr[size];
  // EXPECT_EQ((char)0, c);
}
}
