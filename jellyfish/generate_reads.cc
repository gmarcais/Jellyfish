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
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>

#include <jellyfish/err.hpp>
#include <jellyfish/dbg.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/randomc.h>
#include <jellyfish/generate_reads_cmdline.hpp>

class generate_reads_t {
  size_t           _int_seq_len;
  size_t           _seq_len;
  int              _read_len;
  int              _int_read_len;
  int              _rest_read_len;
  int              _expected_error;
  CRandomMersenne *_rng;
  uint32_t        *_seq;
  uint32_t        *_read;

public:
  generate_reads_t(size_t seq_len, int read_len, double error_rate, CRandomMersenne &rng) :
    _int_seq_len(seq_len / 16), _seq_len(_int_seq_len * 16),
    _read_len(read_len), _int_read_len(_read_len / 16),
    _rest_read_len(read_len % 16),
    _expected_error(lround(error_rate * _read_len)),
    _rng(&rng)
  {
    _seq  = new uint32_t[1 + _int_seq_len];
    _read = new uint32_t[1 + _int_read_len];
  }

  ~generate_reads_t() {
    delete[] _read;
    delete[] _seq;
  }

  void generate_sequence() {
    uint32_t * const end = _seq + _int_seq_len;

    for(uint32_t *cur = _seq; cur < end; ++cur)
      *cur = _rng->BRandom();
  }

  void dump_sequence(std::ostream &os) {
    static const int line_len = 4;
    os << ">genome " << _seq_len << "\n";
    size_t i;
    for(i = 0; i < _int_seq_len; i += line_len) {
      dump_chunk(_seq + i, line_len, 0, os);
      os << "\n";
    }
    size_t rest = _int_seq_len % line_len;
    if(rest) {
      dump_chunk(_seq + i, rest , 0, os);
      os << "\n";
    }
  }

  void dump_chunk(uint32_t *base, size_t int_len, size_t rest_len,
                  std::ostream &os) {
    static const char letters[4] = { 'A', 'C', 'G', 'T' };
    uint32_t *cur;
    for(cur = base; cur < base + int_len; ++cur) {
      os << letters[ *cur        & 0x3] << letters[(*cur >> 2)  & 0x3]
         << letters[(*cur >> 4)  & 0x3] << letters[(*cur >> 6)  & 0x3]
         << letters[(*cur >> 8)  & 0x3] << letters[(*cur >> 10) & 0x3]
         << letters[(*cur >> 12) & 0x3] << letters[(*cur >> 14) & 0x3]
         << letters[(*cur >> 16) & 0x3] << letters[(*cur >> 18) & 0x3]
         << letters[(*cur >> 20) & 0x3] << letters[(*cur >> 22) & 0x3]
         << letters[(*cur >> 24) & 0x3] << letters[(*cur >> 26) & 0x3]
         << letters[(*cur >> 28) & 0x3] << letters[(*cur >> 30) & 0x3];
    }
    uint32_t bases = *cur;
    switch(rest_len) {
    case 15: os << letters[bases & 0x3]; bases >>= 2;
    case 14: os << letters[bases & 0x3]; bases >>= 2;
    case 13: os << letters[bases & 0x3]; bases >>= 2;
    case 12: os << letters[bases & 0x3]; bases >>= 2;
    case 11: os << letters[bases & 0x3]; bases >>= 2;
    case 10: os << letters[bases & 0x3]; bases >>= 2;
    case 9:  os << letters[bases & 0x3]; bases >>= 2;
    case 8:  os << letters[bases & 0x3]; bases >>= 2;
    case 7:  os << letters[bases & 0x3]; bases >>= 2;
    case 6:  os << letters[bases & 0x3]; bases >>= 2;
    case 5:  os << letters[bases & 0x3]; bases >>= 2;
    case 4:  os << letters[bases & 0x3]; bases >>= 2;
    case 3:  os << letters[bases & 0x3]; bases >>= 2;
    case 2:  os << letters[bases & 0x3]; bases >>= 2;
    case 1:  os << letters[bases & 0x3]; bases >>= 2;
    }
  }

  void add_errors() {
    static const uint32_t patterns[48] = {
      1, 2, 3, 4, 8, 12, 16, 32, 48, 64, 128, 192, 
      256, 512, 768, 1024, 2048, 3072, 4096, 8192, 12288, 
      16384, 32768, 49152, 65536, 131072, 196608, 
      262144, 524288, 786432, 1048576, 2097152, 3145728, 
      4194304, 8388608, 12582912, 16777216, 33554432, 50331648,
      67108864, 134217728, 201326592, 268435456, 536870912, 805306368,
      1073741824, 2147483648, 3221225472 };

    const int actual_nb_error = _rng->IRandom(0, _expected_error);
    for(int i = 0; i < actual_nb_error; ++i) {
      int error_pos         = _rng->IRandom(0, _read_len - 1);
      int int_error_pos     = error_pos / 16;
      int pat_max           = int_error_pos == _int_read_len ? 3 * _rest_read_len : 48;
      *(_read + int_error_pos) ^= patterns[_rng->IRandom(0, pat_max - 1)];
    }
  }

  void generate_reads(long nb, std::ostream &os) {
    for(long i = 0; i < nb; ++i) {
      os << ">read" << i << "\n";
      uint32_t *base = 
        _seq + _rng->IRandom(0, _int_seq_len - _int_read_len - 1);
      // TODO Fix: reads start only every 16 bases!
      memcpy(_read, base, 
             sizeof(uint32_t) * (_int_read_len + (_rest_read_len != 0)));
      add_errors();
      dump_chunk(_read, _int_read_len, _rest_read_len, os);
      os << "\n";
    }
  }

};

int main(int argc, char *argv[])
{
  generate_reads_args args();
  if(generate_reads_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed!";

  CRandomMersenne rng(args.seed_arg);
  generate_reads_t generator(args.genome_length_arg, args.read_length_arg,
                             args.error_rate_arg, rng);
  std::ofstream output(args.output_arg);
  if(!output.good())
    die << "'" << args.output_arg << "' can't open for writing" << err::no;
  
  generator.generate_sequence();
  if(args.sequence_given) {
    std::ofstream sequence(args.sequence_arg);
    if(!sequence.good())
      die << "'" << args.sequence_arg << "' can't open for writing" << err::no;
    generator.dump_sequence(sequence);
    sequence.close();
  }

  long nb_reads =
    args.genome_length_arg * args.coverage_arg / args.read_length_arg;
  generator.generate_reads(nb_reads, output);
  output.close();

  return 0;
}
