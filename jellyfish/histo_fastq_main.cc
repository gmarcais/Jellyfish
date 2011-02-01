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
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/fastq_dumper.hpp>

/*
 * Option parsing
 */
static char doc[] = "Create an histogram of k-mer occurences";
static char args_doc[] = "database";

static struct argp_option options[] = {
  {"low",               'l',    "LOW",  0,      "Low count value of histogram (0.0)"},
  {"high",              'h',    "HIGH", 0,      "High count value of histogram (10000.0)"},
  {"increment",         'i',    "INC",  0,      "Increment for value of histogram (1.0)"},
  { 0 }
};

struct arguments {
  float    low, high, increment;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;
  error_t error;

#define ULONGP(field) \
  error = parse_long(arg, &std::cerr, &arguments->field);       \
  if(error) return(error); else break;

#define UFLOATP(field) \
  error = parse_float(arg, &std::cerr, &arguments->field);       \
  if(error) return(error); else break;

#define FLAG(field) arguments->field = true; break;

  switch(key) {
  case 'l': UFLOATP(low);
  case 'h': UFLOATP(high);
  case 'i': UFLOATP(increment);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int histo_fastq_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.low       = 0.0;
  arguments.high      = 10000.0;
  arguments.increment = 1.0;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }
  if(arguments.low < 0.0) {
    fprintf(stderr, "Low count value must be >= 0.0\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  fastq_storage_t *hash = raw_fastq_dumper_t::read(argv[arg_st]);
  fastq_storage_t::iterator it = hash->iterator_all();
  const float base = 
    arguments.low > 0.0 ? (arguments.increment >= arguments.low ? 0.0 : arguments.low - arguments.increment) : 0.0;
  const float ceil = arguments.high + arguments.increment;
  const int nb_buckets = (int)ceilf((ceil + arguments.increment - base) / arguments.increment);
  const int last_bucket = nb_buckets - 1;
  std::vector<uint64_t> histogram(nb_buckets, 0UL);

  while(it.next()) {
    int i = (int)roundf((it.get_val().to_float() - base) / arguments.increment);
    if(i < 0) {
      histogram[0]++;
    } else if(i >= last_bucket) {
      histogram[last_bucket]++;
    } else {
      histogram[i]++;
    }
  }

  for(int i = 0; i < nb_buckets; i++)
    std::cout << (base + (float)i * arguments.increment) <<
      " " << histogram[i] << "\n";

  std::cout << std::flush;

  return 0;
}
