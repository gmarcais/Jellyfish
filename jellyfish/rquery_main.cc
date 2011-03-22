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

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/compacted_hash.hpp>
#include <jellyfish/rquery_main_cmdline.hpp>

int raw_query_main(int argc, char *argv[])
{
  struct rquery_main_args args;

  if(rquery_main_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  if(args.inputs_num != 1)
    die << "Need 1 database\n"
        << rquery_main_args_usage << "\n" << rquery_main_args_help;

  mapped_file dbf(args.inputs[0]);
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(strncmp(type, "JFRHSHDN", sizeof(8)))
    die << "Invalid database type '" << err::substr(dbf.base(), 8) 
        << "', expected 'JFRHSHDN'";
  inv_hash_storage_t hash(dbf.base() + 8, dbf.length() - 8);

  uint_t mer_len = hash.get_key_len() / 2;
  const uint_t width = mer_len + 2;
  if(args.verbose_flag)
    std::cerr << "k-mer length (bases): " << (hash.get_key_len() / 2) << "\n"
              << "value length (bits):  " << hash.get_val_len() << "\n";

  std::ofstream out(args.output_arg);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'" << err::no;

  char mer[width + 2];
  char mer2[width + 2];
  uint64_t key;
  uint64_t val;
  while(true) {
    std::cin >> std::setw(width) >> mer;
    if(!std::cin.good())
      break;
    if(strlen(mer) != mer_len) {
      die << "Invalid mer %s\n" <<  (char*)mer;
    }
    key = jellyfish::parse_dna::mer_string_to_binary(mer, mer_len);
    if(args.both_strands_flag) {
      uint64_t rev = jellyfish::parse_dna::reverse_complement(key, mer_len);
      if(rev < key)
        key = rev;
    }
    if(!hash.get_val(key, val, true))
      val = 0;
    jellyfish::parse_dna::mer_binary_to_string(key, mer_len, mer2);
    out << mer << " " << mer2 << " " << val << "\n";
  }
  out.close();
  
  return 0;
}
