/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_PARSER_COMMON_H__
#define __JELLYFISH_PARSER_COMMON_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>

#include <jellyfish/sam_format.hpp>

namespace jellyfish {
enum file_type { DONE_TYPE
                 , FASTA_TYPE
                 , FASTQ_TYPE
#ifdef HAVE_HTSLIB
                 , SAM_TYPE
#endif
};

struct stream_type {
  std::unique_ptr<std::istream> standard;
#ifdef HAVE_HTSLIB
  std::unique_ptr<sam_wrapper> sam;
#endif

  bool good() {
    return (standard && standard->good())
#ifdef HAVE_HTSLIB
      || (sam && sam->good())
#endif
      ;
  }

  void reset() {
    standard.reset();
#ifdef HAVE_HTSLIB
    sam.reset();
#endif
  }
};

}

#endif // __JELLYFISH_PARSER_COMMON_H__
