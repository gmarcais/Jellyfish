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
#include <argp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "misc.hpp"
#include "mer_counting.hpp"
#include "compacted_hash.hpp"

/*
 * Option parsing
 */
static char doc[] = "Create an histogram of k-mer occurences";
static char args_doc[] = "database";

static struct argp_option options[] = {
  {"buffer-size",       's',    "LEN",  0,      "Length in bytes of input buffer (10MB)"},
  {"low",               'l',    "LOW",  0,      "Low count value of histogram (1)"},
  {"high",              'h',    "HIGH", 0,      "High count value of histogram (10000)"},
  {"increment",         'i',    "INC",  0,      "Increment for value of histogram"},
  {"verbose",           'v',    0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  uint64_t low, high, increment;
  size_t   buff_size;
  bool     verbose;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;
  error_t error;

#define ULONGP(field) \
  error = parse_long(arg, &std::cerr, &arguments->field);       \
  if(error) return(error); else break;

#define FLAG(field) arguments->field = true; break;

  switch(key) {
  case 's': ULONGP(buff_size);
  case 'l': ULONGP(low);
  case 'h': ULONGP(high);
  case 'i': ULONGP(increment);
  case 'v': FLAG(verbose);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int histo_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.buff_size = 10000000;
  arguments.low       = 1;
  arguments.high      = 10000;
  arguments.increment = 1;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }
  if(arguments.low < 1) {
    fprintf(stderr, "Low count value must be >= 1\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  hash_reader_t hash(argv[arg_st]);
  const uint64_t base = 
    arguments.low > 1 ? (arguments.increment >= arguments.low ? 1 : arguments.low - arguments.increment) : 1;
  const uint64_t ceil = arguments.high + arguments.increment;
  const uint64_t nb_buckets = (ceil + arguments.increment - base) / arguments.increment;
  const uint64_t last_bucket = nb_buckets - 1;
  std::vector<uint64_t> histogram(nb_buckets, 0UL);

  while(hash.next()) {
    if(hash.val < base) {
      histogram[0]++;
    } else if(hash.val > ceil) {
      histogram[last_bucket]++;
    } else {
      histogram[(hash.val - base) / arguments.increment]++;
    }
  }

  for(uint64_t i = 0; i < nb_buckets; i++)
    std::cout << (base + i * arguments.increment) <<
      " " << histogram[i] << "\n";

  std::cout << std::flush;

  return 0;
}
