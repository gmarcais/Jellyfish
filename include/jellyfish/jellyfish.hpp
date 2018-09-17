/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_JELLYFISH_HPP__
#define __JELLYFISH_JELLYFISH_HPP__

#include <stdint.h>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/text_dumper.hpp>
#include <jellyfish/binary_dumper.hpp>

typedef jellyfish::cooperative::hash_counter<jellyfish::mer_dna> mer_hash;
typedef mer_hash::array mer_array;
typedef jellyfish::text_dumper<mer_array> text_dumper;
typedef jellyfish::text_reader<jellyfish::mer_dna, uint64_t> text_reader;
typedef jellyfish::binary_dumper<mer_array> binary_dumper;
typedef jellyfish::binary_reader<jellyfish::mer_dna, uint64_t> binary_reader;
typedef jellyfish::binary_query_base<jellyfish::mer_dna, uint64_t> binary_query;
typedef jellyfish::binary_writer<jellyfish::mer_dna, uint64_t> binary_writer;
typedef jellyfish::text_writer<jellyfish::mer_dna, uint64_t> text_writer;


#endif /* __JELLYFISH_JELLYFISH_HPP__ */
