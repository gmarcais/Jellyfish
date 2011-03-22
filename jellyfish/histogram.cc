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
#include <vector>

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/compacted_hash.hpp>
#include <jellyfish/histogram_cmdline.hpp>

int histo_main(int argc, char *argv[])
{
  struct histogram_args args;

  if(histogram_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";
  if(args.inputs_num != 1)
    die << "Need 1 database\n"
        << histogram_args_usage << "\n" << histogram_args_help;

  if(args.low_arg < 1)
    die << "Low count value must be >= 1\n"
        << histogram_args_usage << "\n" << histogram_args_help;

  hash_reader_t hash(args.inputs[0]);
  const uint64_t base = 
    args.low_arg > 1 ? (args.increment_arg >= args.low_arg ? 1 : args.low_arg - args.increment_arg) : 1;
  const uint64_t ceil = args.high_arg + args.increment_arg;
  const uint64_t nb_buckets = (ceil + args.increment_arg - base) / args.increment_arg;
  const uint64_t last_bucket = nb_buckets - 1;
  std::vector<uint64_t> histogram(nb_buckets, 0UL);

  while(hash.next()) {
    if(hash.val < base) {
      histogram[0]++;
    } else if(hash.val > ceil) {
      histogram[last_bucket]++;
    } else {
      histogram[(hash.val - base) / args.increment_arg]++;
    }
  }

  for(uint64_t i = 0; i < nb_buckets; i++)
    std::cout << (base + i * args.increment_arg) 
              << " " << histogram[i] << "\n";

  std::cout << std::flush;

  return 0;
}
