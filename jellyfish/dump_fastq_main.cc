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
#include <float.h>
#include <iostream>
#include <fstream>

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/fastq_dumper.hpp>
#include <jellyfish/dump_fastq_main_cmdline.hpp>
#include <jellyfish/fstream_default.hpp>

int dump_fastq_main(int argc, char *argv[])
{
  dump_fastq_main_args args(argc, argv);

  ofstream_default out(args.output_given ? args.output_arg : 0, std::cout);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'" << err::no;

  fastq_storage_t *hash =
    raw_fastq_dumper_t::read(args.db_arg);
  if(args.verbose_flag)
    std::cerr << 
      "k-mer length (bases): " << (hash->get_key_len() / 2) << "\n"
      "entries             : " << hash->get_size() << "\n";
  
  float lower_count = args.lower_count_given ? args.lower_count_arg : 0;
  float upper_count = args.upper_count_given ? args.upper_count_arg : FLT_MAX;

  fastq_storage_t::iterator it = hash->iterator_all();
  out << std::scientific;
  if(args.column_flag) {
    char spacer = args.tab_flag ? '\t' : ' ';
    while(it.next()) {
      float val = it.get_val().to_float();
      if(val < lower_count || val > upper_count)
        continue;
      out << it.get_dna_str() << spacer << val << "\n";
    }
  } else {
    while(it.next()) {
      float val = it.get_val().to_float();
      if(val < lower_count || val > upper_count)
        continue;
      out << ">" << val << "\n" << it.get_dna_str() << "\n";
    }
  }
  out.close();

  return 0;
}
