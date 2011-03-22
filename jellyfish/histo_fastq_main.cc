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
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/fastq_dumper.hpp>
#include <jellyfish/histo_fastq_main_cmdline.hpp>

int histo_fastq_main(int argc, char *argv[])
{
  struct histo_fastq_main_args args;

  if(histo_fastq_main_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  if(args.inputs_num != 1)
    die << "Need 1 database\n"
        << histo_fastq_main_args_usage << "\n"
        << histo_fastq_main_args_help;

  if(args.low_arg < 0.0)
    die << "Low count (" << args.low_arg << ") must be >= 0.0\n"
        << histo_fastq_main_args_usage << "\n"
        << histo_fastq_main_args_help;

  fastq_storage_t *hash = raw_fastq_dumper_t::read(args.inputs[0]);
  fastq_storage_t::iterator it = hash->iterator_all();
  const float base = 
    args.low_arg > 0.0 ? (args.increment_arg >= args.low_arg ? 0.0 : args.low_arg - args.increment_arg) : 0.0;
  const float ceil = args.high_arg + args.increment_arg;
  const int nb_buckets = (int)ceilf((ceil + args.increment_arg - base) / args.increment_arg);
  const int last_bucket = nb_buckets - 1;
  std::vector<uint64_t> histogram(nb_buckets, 0UL);

  while(it.next()) {
    int i = (int)roundf((it.get_val().to_float() - base) / args.increment_arg);
    if(i < 0) {
      histogram[0]++;
    } else if(i >= last_bucket) {
      histogram[last_bucket]++;
    } else {
      histogram[i]++;
    }
  }

  for(int i = 0; i < nb_buckets; i++)
    std::cout << (base + (float)i * args.increment_arg) <<
      " " << histogram[i] << "\n";

  std::cout << std::flush;

  return 0;
}
