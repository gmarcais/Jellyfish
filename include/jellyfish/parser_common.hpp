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
