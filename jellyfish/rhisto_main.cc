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
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/compacted_hash.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/counter.hpp>

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
  {"threads",           't',    "NB",   0,      "Number of threads (1)"},
  { 0 }
};

struct arguments {
  uint64_t low, high, increment;
  size_t   buff_size;
  uint64_t threads;
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
  case 't': ULONGP(threads);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

class histogram : public thread_exec {
  const inv_hash_t *hash;
  const uint_t      threads;
  const uint64_t    base, ceil, inc, nb_buckets, nb_slices;
  uint64_t         *data;
  counter_t         slice_id;

public:
  histogram(const inv_hash_t *_hash, uint_t _threads, uint64_t _base,
            uint64_t _ceil, uint64_t _inc) :
    hash(_hash), threads(_threads), base(_base), ceil(_ceil), inc(_inc), 
    nb_buckets((ceil + inc - base) / inc), nb_slices(threads * 100)
  {
    data = new uint64_t[threads * nb_buckets];
    memset(data, '\0', threads * nb_buckets * sizeof(uint64_t));
  }

  ~histogram() {
    if(data)
      delete [] data;
  }

  void do_it() {
    exec_join(threads);
  }

  void start(int th_id) {
    uint64_t *hist = &data[th_id * nb_buckets];
    for(size_t i = slice_id++; i <= nb_slices; i = slice_id++) {
      inv_hash_t::iterator it = hash->iterator_slice(i, nb_slices);
      while(it.next()) {
        if(it.get_val() < base)
          hist[0]++;
        else if(it.get_val() > ceil)
          hist[nb_buckets - 1]++;
        else
          hist[(it.get_val() - base) / inc]++;
      }
    }
  }

  void print(std::ostream &out) {
    uint64_t col = base;
    for(uint64_t i = 0; i < nb_buckets; i++, col += inc) {
      uint64_t count = 0;
      for(uint_t j = 0; j < threads; j++)
        count += data[j * nb_buckets + i];
      out << col << " " << count << "\n";
    }
  }
};

int raw_histo_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.buff_size = 10000000;
  arguments.low       = 1;
  arguments.high      = 10000;
  arguments.increment = 1;
  arguments.threads   = 1;
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

  mapped_file dbf(argv[arg_st]);
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(strncmp(type, "JFRHSHDN", sizeof(8)))
    die("Invalid database type '%.8s', expected '%.8s'",
        dbf.base(), "JFRHSHDN");
  dbf.sequential().will_need();
  inv_hash_t hash(dbf.base() + 8, dbf.length() - 8);

  const uint64_t base = 
    arguments.low > 1 ? (arguments.increment >= arguments.low ? 1 : arguments.low - arguments.increment) : 1;
  const uint64_t ceil = arguments.high + arguments.increment;

  histogram histo(&hash, arguments.threads, base, ceil, arguments.increment);
  histo.do_it();
  histo.print(std::cout);
  std::cout << std::flush;

  return 0;
}
