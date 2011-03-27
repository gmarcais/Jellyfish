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
#include <jellyfish/dump_stats_compacted_cmdline.hpp>

int stats_main(int argc, char *argv[])
{
  struct dump_stats_compacted_args args;

  if(dump_stats_compacted_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  if(args.inputs_num != 1)
    die << "Need 1 database\n"
        << dump_stats_compacted_args_usage << "\n"
        << dump_stats_compacted_args_help;

  hash_reader_t hash(args.inputs[0]);
  if(args.verbose_flag)
    std::cerr << "k-mer length (bases): " << (hash.get_key_len() / 2) << "\n"
              << "value length (bytes): " << hash.get_val_len() << "\n";

  std::ofstream out(args.output_arg);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'";

  if(!(args.fasta_flag || args.column_flag) && !args.recompute_flag) {
    out << "Unique:    " << hash.get_unique() << "\n"
        << "Distinct:  " << hash.get_distinct() << "\n"
        << "Total:     " << hash.get_total() << "\n" 
        << "Max Count: " << hash.get_max_count() << "\n";
    return 0;
  }

  uint64_t lower_count = args.lower_count_given ? args.lower_count_arg : 1;
  uint64_t upper_count = args.upper_count_given ? args.upper_count_arg : (uint64_t)-1;
  if(args.fasta_flag) {
    char key[hash.get_mer_len() + 1];
    while(hash.next()) {
      if(hash.val < lower_count || hash.val > upper_count)
        continue;
      hash.get_string(key);
      out << ">" << hash.val << "\n" << key << "\n";
    }
  } else if(args.column_flag) {
    char key[hash.get_mer_len() + 1];
    char spacer = args.tab_flag ? '\t' : ' ';
    while(hash.next()) {
      if(hash.val < lower_count || hash.val > upper_count)
        continue;
      hash.get_string(key);
      out << key << spacer << hash.val << "\n";
    }
  } else {
    uint64_t unique = 0, distinct = 0, total = 0, max_count = 0;
    while(hash.next()) {
      if(hash.val < lower_count || hash.val > upper_count)
        continue;
      unique += hash.val == 1;
      distinct++;
      total += hash.val;
      if(hash.val > max_count)
        max_count = hash.val;
    }
    out << "Unique:    " << unique << "\n"
        << "Distinct:  " << distinct << "\n"
        << "Total:     " << total << "\n"
        << "Max_count: " << max_count << "\n";
  }
  out.close();

  return 0;
}
