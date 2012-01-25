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
#include <jellyfish/dump_main_cmdline.hpp>
#include <jellyfish/fstream_default.hpp>

template<typename hash_t>
void dump(const hash_t &h, std::ostream &out,
          const dump_args &args, uint64_t lower_count, uint64_t upper_count) {
  typename hash_t::iterator it = h.iterator_all();
  if(args.column_flag) {
    char spacer = args.tab_flag ? '\t' : ' ';
    while(it.next()) {
      if(it.get_val() < lower_count || it.get_val() > upper_count)
        continue;
      out << it.get_dna_str() << spacer << it.get_val() << "\n";
    }
  } else {
    while(it.next()) {
      if(it.get_val() < lower_count || it.get_val() > upper_count)
        continue;
      out << ">" << it.get_val() << "\n" << it.get_dna_str() << "\n";
    }
  }
}

int dump_main(int argc, char *argv[])
{
  dump_args args(argc, argv);

  ofstream_default out(args.output_given ? args.output_arg : 0, std::cout);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'";

  mapped_file dbf(args.db_arg.c_str());
  dbf.sequential().will_need();
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));

  uint64_t lower_count = args.lower_count_given ? args.lower_count_arg : 0;
  uint64_t upper_count = args.upper_count_given ? args.upper_count_arg : (uint64_t)-1;

  if(!strncmp(type, jellyfish::compacted_hash::file_type, sizeof(type))) {
    hash_query_t hash(dbf);
    dump(hash, out, args, lower_count, upper_count);
  } else if(!strncmp(type, jellyfish::raw_hash::file_type, sizeof(type))) {
    raw_inv_hash_query_t hash(dbf);
    dump(hash, out, args, lower_count, upper_count);
  } else {
    die << "Invalid file type '" << err::substr(type, sizeof(type)) << "'.";
  }
  out.close();

  return 0;
}
