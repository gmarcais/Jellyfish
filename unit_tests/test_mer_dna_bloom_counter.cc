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


#include <stdlib.h>
#include <algorithm>
#include <utility>
#include <set>
#include <string>
#include <gtest/gtest.h>
#include <jellyfish/mer_dna_bloom_counter.hpp>

namespace {
using jellyfish::mer_dna;
using jellyfish::mer_dna_bloom_counter;

TEST(MerDnaBloomCounter, FalsePositive) {
  mer_dna::k(50);
  static const size_t nb_inserts = 10000;
  static const double error_rate = 0.001;
  std::set<mer_dna> mer_set;
  mer_dna_bloom_counter bc(error_rate, nb_inserts);

  size_t collisions2 = 0;
  size_t collisions3 = 0;

  // Insert once nb_inserts. Insert twice the first half
  {
    // First insertion
    size_t nb_collisions   = 0;
    mer_dna m;
    for(size_t i = 0; i < nb_inserts; ++i) {
      m.randomize();
      mer_set.insert(m);
      nb_collisions += bc.insert(m) > 0;
    }
    EXPECT_GT(error_rate * nb_inserts, nb_collisions);
  }

  // Second insertion
  {
    size_t nb_collisions = 0;
    size_t nb_errors     = 0;
    auto it = mer_set.cbegin();
    for(size_t i =0; i < nb_inserts / 2; ++i, ++it) {
      unsigned int oc = bc.insert(*it);
      nb_collisions += oc > 1;
      nb_errors += oc < 1;
    }
    EXPECT_GT(2 * error_rate * nb_inserts, nb_collisions);
  }

  // Check known mers
  {
    size_t nb_collisions = 0;
    size_t nb_errors     = 0;
    auto it = mer_set.cbegin();
    for(size_t i = 0; i < nb_inserts; ++i, ++it) {
      if(i < nb_inserts / 2) {
        nb_errors += bc.check(*it) < 2;
      } else {
        nb_errors += bc.check(*it) < 1;
        nb_collisions += bc.check(*it) > 1;
      }
    }
    EXPECT_EQ((size_t)0, nb_errors);
    EXPECT_GT(2 * error_rate * nb_inserts, nb_collisions);
  }

  // Check unknown mers
  {
    size_t nb_collisions = 0;
    mer_dna m;
    for(size_t i = 0; i < nb_inserts; ++i) {
      m.randomize();
      nb_collisions += bc.check(m) > 1;
    }
    std::cout << nb_collisions << "\n";
    EXPECT_GT(2 * error_rate * nb_inserts, nb_collisions);
  }

  // // Check known strings
  // for(size_t i = 0; i < nb_strings; ++i) {
  //   std::vector<elt>::reference ref = counts[i];
  //   unsigned int expected = std::min(ref.second, (unsigned int)2);
  //   unsigned int actual = bc1.check(ref.first.c_str());
  //   EXPECT_LE(expected, actual);
  //   if(expected != actual)
  //     ++nb_errors;
  //   if(expected != *bc2[ref.first.c_str()])
  //     ++collisions2;

  //   if(expected != *bc3[ref.first.c_str()])
  //     ++collisions3;
  // }
  // EXPECT_GT(error_rate * nb_strings, nb_errors);
  // EXPECT_GT(error_rate * nb_strings, collisions2);
  // EXPECT_GT(error_rate * nb_strings, collisions3);

  // nb_errors = collisions2 = collisions3 = 0;
  // // Check unknown strings
  // for(size_t i = 0; i < nb_inserts; ++i) {
  //   for(size_t j = 0; j < str_len; ++j)
  //     str[j] = '0' + (random() % 10);
  //   if(bc1.check(str) > 0)      
  //     ++nb_errors;
  //   if(bc2[str] > 0)
  //     ++collisions2;
  //   if(bc3[str] > 0)
  //     ++collisions3;
  // }
  // EXPECT_GT(2 * error_rate * nb_inserts, nb_errors);
  // EXPECT_GT(2 * error_rate * nb_inserts, collisions2);
  // EXPECT_GT(2 * error_rate * nb_inserts, collisions3);
}
}
