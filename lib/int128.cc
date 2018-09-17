/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_INT128
#include <jellyfish/int128.hpp>

void __int128_ns::__print_bases(std::ostream& prefix, std::ostream& os,
                                unsigned __int128 x, 
                                const std::ios::fmtflags& ff) {
  if(x == 0) {
    os << "0";
    return;
  }
  if(ff & std::ios::showbase) {
    if(ff & std::ios::hex) {
      if(ff & std::ios::uppercase)
        prefix << "0X";
      else
        prefix << "0x";
    } else if(ff & std::ios::oct) {
      prefix << "0";
    }
  }
  if(ff & std::ios::hex) {
    __print_digits<16>(os, (unsigned __int128)x,
                       !(ff & std::ios::uppercase));
  } else if(ff & std::ios::oct) {
    __print_digits<8>(os, (unsigned __int128)x);
  }
}

#ifndef HAVE_NUMERIC_LIMITS128
const int std::numeric_limits<__int128>::digits;
const int std::numeric_limits<__int128>::digits10;
const bool std::numeric_limits<__int128>::is_signed;
const bool std::numeric_limits<__int128>::is_integer;
const bool std::numeric_limits<__int128>::is_exact;
const int std::numeric_limits<__int128>::radix;
const int std::numeric_limits<__int128>::min_exponent;
const int std::numeric_limits<__int128>::min_exponent10;
const int std::numeric_limits<__int128>::max_exponent;
const int std::numeric_limits<__int128>::max_exponent10;
const bool std::numeric_limits<__int128>::has_infinity;
const bool std::numeric_limits<__int128>::has_quiet_NaN;
const bool std::numeric_limits<__int128>::has_signaling_NaN;
const std::float_denorm_style std::numeric_limits<__int128>::has_denorm;
const bool std::numeric_limits<__int128>::has_denorm_loss;
const bool std::numeric_limits<__int128>::is_iec559;
const bool std::numeric_limits<__int128>::is_bounded;
const bool std::numeric_limits<__int128>::is_modulo;
const bool std::numeric_limits<__int128>::traps;
const bool std::numeric_limits<__int128>::tinyness_before;
const std::float_round_style std::numeric_limits<__int128>::round_style;

const int std::numeric_limits<unsigned __int128>::digits;
const int std::numeric_limits<unsigned __int128>::digits10;
const bool std::numeric_limits<unsigned __int128>::is_signed;
const bool std::numeric_limits<unsigned __int128>::is_integer;
const bool std::numeric_limits<unsigned __int128>::is_exact;
const int std::numeric_limits<unsigned __int128>::radix;
const int std::numeric_limits<unsigned __int128>::min_exponent;
const int std::numeric_limits<unsigned __int128>::min_exponent10;
const int std::numeric_limits<unsigned __int128>::max_exponent;
const int std::numeric_limits<unsigned __int128>::max_exponent10;
const bool std::numeric_limits<unsigned __int128>::has_infinity;
const bool std::numeric_limits<unsigned __int128>::has_quiet_NaN;
const bool std::numeric_limits<unsigned __int128>::has_signaling_NaN;
const std::float_denorm_style std::numeric_limits<unsigned __int128>::has_denorm;
const bool std::numeric_limits<unsigned __int128>::has_denorm_loss;
const bool std::numeric_limits<unsigned __int128>::is_iec559;
const bool std::numeric_limits<unsigned __int128>::is_bounded;
const bool std::numeric_limits<unsigned __int128>::is_modulo;
const bool std::numeric_limits<unsigned __int128>::traps;
const bool std::numeric_limits<unsigned __int128>::tinyness_before;
const std::float_round_style std::numeric_limits<unsigned __int128>::round_style;
#endif 
#endif
