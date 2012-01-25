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
#include <jellyfish/stats_main_cmdline.hpp>
#include <jellyfish/fstream_default.hpp>

template<typename hash_t>
void compute_stats(const hash_t &h, uint64_t lower_count, uint64_t upper_count,
                   uint64_t &unique, uint64_t &distinct, uint64_t &total,
                   uint64_t &max_count) {
  typename hash_t::iterator it = h.iterator_all();
  while(it.next()) {
    if(it.get_val() < lower_count || it.get_val() > upper_count)
      continue;
    unique += it.get_val() == 1;
    distinct++;
    total += it.get_val();
    if(it.get_val() > max_count)
      max_count = it.get_val();
  }
}

int stats_main(int argc, char *argv[])
{
  stats_args args(argc, argv);

  ofstream_default out(args.output_given ? args.output_arg : 0, std::cout);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'";

  mapped_file dbf(args.db_arg);
  dbf.sequential().will_need();
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));

  args.recompute_flag |= args.lower_count_given | args.upper_count_given;
  uint64_t lower_count = args.lower_count_given ? args.lower_count_arg : 0;
  uint64_t upper_count = args.upper_count_given ? args.upper_count_arg : (uint64_t)-1;

  uint64_t unique = 0, distinct = 0, total = 0, max_count = 0;
  if(!strncmp(type, jellyfish::compacted_hash::file_type, sizeof(type))) {
    hash_query_t hash(dbf);
    if(args.verbose_flag)
      out << "k-mer length (bases): " << hash.get_mer_len() << "\n"
          << "value length (bytes): " << hash.get_val_len() << "\n";

    if(args.recompute_flag) {
      compute_stats(hash, lower_count, upper_count, unique, distinct,
                    total, max_count);
    } else {
      unique    = hash.get_unique();
      distinct  = hash.get_distinct();
      total     = hash.get_total();
      max_count = hash.get_max_count();
    }
  } else if(!strncmp(type, jellyfish::raw_hash::file_type, sizeof(type))) {
    raw_inv_hash_query_t hash(dbf);
    if(args.verbose_flag)
      out << "k-mer length (bases): " << hash.get_mer_len() << "\n"
          << "value length (bits) : " << hash.get_val_len() << "\n";
    compute_stats(hash, lower_count, upper_count, unique, distinct,
                  total, max_count);
  } else {
    die << "Invalid file type '" << err::substr(type, sizeof(type)) << "'.";
  }
  out << "Unique:    " << unique << "\n"
      << "Distinct:  " << distinct << "\n"
      << "Total:     " << total << "\n"
      << "Max_count: " << max_count << "\n";
  out.close();

  // if(args.fasta_flag) {
  //   char key[hash.get_mer_len() + 1];
  //   while(hash.next()) {
  //     if(hash.val < lower_count || hash.val > upper_count)
  //       continue;
  //     hash.get_string(key);
  //     out << ">" << hash.val << "\n" << key << "\n";
  //   }
  // } else if(args.column_flag) {
  //   char key[hash.get_mer_len() + 1];
  //   char spacer = args.tab_flag ? '\t' : ' ';
  //   while(hash.next()) {
  //     if(hash.val < lower_count || hash.val > upper_count)
  //       continue;
  //     hash.get_string(key);
  //     out << key << spacer << hash.val << "\n";
  //   }
  // } else {
  //   die << "Invalid file type '" << err::substr(type, sizeof(type)) << "'.";
  // }
  // out.close();

  return 0;
}
