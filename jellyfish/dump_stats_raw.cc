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
#include <jellyfish/dump_stats_raw_cmdline.hpp>

int raw_stats_main(int argc, char *argv[])
{
  struct dump_stats_raw_args args;

  if(dump_stats_raw_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  if(args.inputs_num != 1)
    die << "Need 1 database\n"
        << dump_stats_raw_args_usage << "\n" << dump_stats_raw_args_help;

  mapped_file dbf(args.inputs[0]);
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(strncmp(type, "JFRHSHDN", sizeof(8)))
    die << "Invalid database type '" << err::substr(dbf.base(), 8) << "', expected 'JFRHSHDN'";

  inv_hash_t hash(dbf.base() + 8, dbf.length() - 8);
  if(args.verbose_flag)
    std::cerr << "k-mer length (bases): " << (hash.get_key_len() / 2) << "\n"
              << "value length (bits):  " << hash.get_val_len() << "\n";

  std::ofstream out(args.output_arg);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'";

  inv_hash_t::iterator it = hash.iterator_all();
  if(args.fasta_flag) {
    while(it.next()) {
      out << ">" << it.get_val() << "\n" << it.get_dna_str() << "\n";
    }
  } else if(args.column_flag) {
    char spacer = args.tab_flag ? '\t' : ' ';
    while(it.next())
      out << it.get_dna_str() << spacer << it.get_val() << "\n";
  } else {
    uint64_t unique = 0, distinct = 0, total = 0, max_count = 0;
    while(it.next()) {
      unique += it.get_val() == 1;
      distinct++;
      total += it.get_val();
      if(it.get_val() > max_count)
        max_count = it.get_val();
    }
    out << "Unique:    " << unique << "\n"
        << "Distinct:  " << distinct << "\n"
        << "Total:     " << total << "\n"
        << "Max_count: " << max_count << "\n";
  }
  out.close();

  return 0;
}
