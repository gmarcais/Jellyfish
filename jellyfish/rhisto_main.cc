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
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/counter.hpp>
#include <jellyfish/rhisto_main_cmdline.hpp>


int raw_histo_main(int argc, char *argv[])
{
  struct rhisto_main_args args;

  if(rhisto_main_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  if(args.inputs_num != 1)
    die << "Need 1 database\n"
        << rhisto_main_args_usage << "\n" << rhisto_main_args_help;

  if(args.low_arg < 1)
    die << "Low count value must be >= 1\n"
        << rhisto_main_args_usage << "\n" << rhisto_main_args_help;

  mapped_file dbf(args.inputs[0]);
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(strncmp(type, "JFRHSHDN", sizeof(8)))
    die << "Invalid database type '" << err::substr(dbf.base(), 8)
        << "', expected 'JFRHSHDN'";
  dbf.sequential().will_need();
  inv_hash_t hash(dbf.base() + 8, dbf.length() - 8);

  const uint64_t base = 
    args.low_arg > 1 ? (args.increment_arg >= args.low_arg ? 1 : args.low_arg - args.increment_arg) : 1;
  const uint64_t ceil = args.high_arg + args.increment_arg;

  histogram histo(&hash, args.threads_arg, base, ceil, args.increment_arg);
  histo.do_it();
  histo.print(std::cout);
  std::cout << std::flush;

  return 0;
}
