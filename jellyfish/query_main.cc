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

int query_main(int argc, char *argv[])
{
  struct query_args args;

  if(query_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  if(args.inputs_num != 1) {
    std::cerr << "Wrong number of argument\n";
    query_cmdline_print_help();
    exit(1);
  }

  std::ofstream out(args.output_arg);
  if(!out.good())
    die << "Can't open output file '" << args.output_arg << "'" << err::no;

  hash_query_t hash(args.inputs[0]);
  hash.set_canonical(args.both_strands_flag);
  uint_t mer_len = hash.get_mer_len();
  const uint_t width = mer_len + 2;
  if(args.verbose_flag) {
    out << "k-mer length (bases):          " <<
      (hash.get_key_len() / 2) << "\n";
    out << "value length (bytes):  " <<
      hash.get_val_len() << "\n";
  }

  char mer[width + 2];
  while(true) {
    std::cin >> std::setw(width) >> mer;
    if(!std::cin.good())
      break;
    if(strlen(mer) != mer_len)
      die << "Invalid mer " << (char*)mer;

    out << mer << " " << hash[mer] << "\n";
  }
  out.close();

  return 0;
}
