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

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/fastq_dumper.hpp>
#include <jellyfish/histo_fastq_main_cmdline.hpp>

int histo_fastq_main(int argc, char *argv[])
{
  histo_fastq_main_args args(argc, argv);

  if(args.low_arg < 0.0)
    args.error("Low count (option --low,-l) must be >= 0.0");

  fastq_storage_t *hash = raw_fastq_dumper_t::read(args.db_arg);
  fastq_storage_t::iterator it = hash->iterator_all();
  float low_arg = args.low_arg;
  float increment_arg = args.increment_arg;
  float high_arg = args.high_arg;
  const float base = 
    low_arg > 0.0 ? (increment_arg >= low_arg ? 0.0 : low_arg - increment_arg) : 0.0;
  const float ceil = high_arg + increment_arg;
  const int nb_buckets = (int)ceilf((ceil + increment_arg - base) / increment_arg);
  const int last_bucket = nb_buckets - 1;
  std::vector<uint64_t> histogram(nb_buckets, 0UL);

  while(it.next()) {
    int i = (int)roundf((it.get_val().to_float() - base) / increment_arg);
    if(i < 0) {
      histogram[0]++;
    } else if(i >= last_bucket) {
      histogram[last_bucket]++;
    } else {
      histogram[i]++;
    }
  }

  for(int i = 0; i < nb_buckets; i++)
    if(histogram[i] > 0.0 || args.full_flag)
      std::cout << (base + (float)i * increment_arg) 
                << " " << histogram[i] << "\n";

  std::cout << std::flush;

  return 0;
}
