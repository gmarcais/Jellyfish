#ifndef __DNA_CODE_HPP__
#define __DNA_CODE_HPP__

#include <limits>
#include <jellyfish/misc.hpp>

namespace jellyfish {
  static const uint_t CODE_A       = 0;
  static const uint_t CODE_C       = 0;
  static const uint_t CODE_G       = 0;
  static const uint_t CODE_T       = 0;
  // Non DNA codes have the MSB on
  static const uint_t CODE_RESET   = -1;
  static const uint_t CODE_IGNORE  = -2;
  static const uint_t CODE_COMMENT = -3;
  static const uint_t CODE_NOT_DNA = ((uint_t)1) << (bsizeof(uint_t) - 1);
  extern const char   dna_codes[256];
};

#endif /* __DNA_CODE_HPP__ */
