/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_MERGE_FILES_HPP__
#define __JELLYFISH_MERGE_FILES_HPP__

#include <vector>
#include <jellyfish/err.hpp>
#include <jellyfish/file_header.hpp>

define_error_class(MergeError);

// Operation on merging
enum merge_op { SUM, MIN, MAX, JACCARD };

/// Merge files. Throw a MergeError in case of error.
void merge_files(std::vector<const char*> input_files, const char* out_file,
                 jellyfish::file_header& h, uint64_t min, uint64_t max,
                 merge_op op = SUM);

#endif /* __JELLYFISH_MERGE_FILES_HPP__ */
