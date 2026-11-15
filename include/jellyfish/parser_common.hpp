/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>
#include <istream>
#include <limits>

#include <jellyfish/sam_format.hpp>

namespace jellyfish {
enum file_type { DONE_TYPE
                 , FASTA_TYPE
                 , FASTQ_TYPE
#ifdef HAVE_HTSLIB
                 , SAM_TYPE
#endif
};

inline void skip_leading_comments(std::istream& is) {
  for(int c = is.peek(); c == ';' || c == '\n' || c == '\r'; c = is.peek()) {
    if(c == ';')
      is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    else
      is.get();
  }
}

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
