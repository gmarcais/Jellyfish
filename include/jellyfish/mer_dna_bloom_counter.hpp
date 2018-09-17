/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_MER_DNA_BLOOM_COUNTER_HPP_
#define __JELLYFISH_MER_DNA_BLOOM_COUNTER_HPP_

#include <jellyfish/mer_dna.hpp>
#include <jellyfish/bloom_counter2.hpp>
#include <jellyfish/bloom_filter.hpp>
#include <jellyfish/rectangular_binary_matrix.hpp>
#include <jellyfish/misc.hpp>

namespace jellyfish {
template<>
struct hash_pair<mer_dna> {
  RectangularBinaryMatrix m1, m2;

  hash_pair() : m1(8 * sizeof(uint64_t), mer_dna::k() * 2), m2(8 * sizeof(uint64_t), mer_dna::k() * 2) {
    m1.randomize(random_bits);
    m2.randomize(random_bits);
  }

  hash_pair(RectangularBinaryMatrix&& m1_, RectangularBinaryMatrix&& m2_) : m1(m1_), m2(m2_) { }

  void operator()(const mer_dna& k, uint64_t* hashes) const {
    hashes[0] = m1.times(k);
    hashes[1] = m2.times(k);
  }
};

typedef bloom_counter2<mer_dna> mer_dna_bloom_counter;
typedef bloom_counter2_file<mer_dna> mer_dna_bloom_counter_file;
typedef bloom_filter<mer_dna> mer_dna_bloom_filter;
typedef bloom_filter_file<mer_dna> mer_dna_bloom_filter_file;
}

#endif /* __JELLYFISH_MER_DNA_BLOOM_COUNTER_HPP_ */
