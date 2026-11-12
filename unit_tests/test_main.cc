/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <random>
#include <limits>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <unit_tests/test_main_cmdline.hpp>
#include <unit_tests/test_main.hpp>
#include <jellyfish/backtrace.hpp>

#include "random_seed.hpp"

// Return length random bits. 1 <= length <= 64
uint64_t random_bits(int length) {
  const uint64_t max = std::numeric_limits<uint64_t>::max() >> (64 - length);
  return std::uniform_int_distribution<uint64_t>(0, max)(prg);
}

std::mt19937_64 prg;

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  cmdline_parse args(argc, argv);

  prg = seeded_prg<std::mt19937_64>("unit_tests_seed", args.seed_given ? args.seed_arg : nullptr);

  std::cout << "Using random seed saved in '" << (args.seed_given ? args.seed_arg : "unit_tests_seed") << '\'' << std::endl;

  if(args.backtrace_flag) {
    show_backtrace();
    setenv("GTEST_CATCH_EXCEPTIONS", "0", 1);
  }

  return RUN_ALL_TESTS();
}
