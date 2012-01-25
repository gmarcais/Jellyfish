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

#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/compacted_hash.hpp>
#include <jellyfish/query_cmdline.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/fstream_default.hpp>

template<typename hash_t>
void print_mer_counts(const hash_t &h, std::istream &in, std::ostream &out) {
  uint_t mer_len = h.get_mer_len();
  uint_t width = mer_len + 2;
  char mer[width + 1];

  while(true) {
    in >> std::setw(width) >> mer;
    if(!in.good())
      break;
    if(strlen(mer) != mer_len)
      die << "Invalid mer " << (char*)mer;

    out << mer << " " << h[mer] << "\n";
  }
}

int query_main(int argc, char *argv[])
{
  query_args args(argc, argv);

  ifstream_default in(args.input_given ? args.input_arg : 0, std::cin);
  if(!in.good())
    die << "Can't open input file '" << args.input_arg << "'" << err::no;
  ofstream_default out(args.input_given ? args.output_arg : 0, std::cout);
  if(!out.good())
    die << "Can't open output file '" << args.output_arg << "'" << err::no;

  mapped_file dbf(args.db_arg.c_str());
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(!strncmp(type, jellyfish::raw_hash::file_type, sizeof(type))) {
    raw_inv_hash_query_t hash(dbf);
    hash.set_canonical(args.both_strands_flag);
    print_mer_counts(hash, in, out);
  } else if(!strncmp(type, jellyfish::compacted_hash::file_type, sizeof(type))) {
    hash_query_t hash(dbf);
    hash.set_canonical(args.both_strands_flag);
    print_mer_counts(hash, in, out);
  }

  out.close();

  return 0;
}
