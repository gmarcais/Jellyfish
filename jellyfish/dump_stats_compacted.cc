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
#include "misc.hpp"
#include "mer_counting.hpp"
#include "compacted_hash.hpp"

/*
 * Option parsing
 */
static char doc[] = "Dump k-mer statistics from a compacted database";
static char args_doc[] = "database";

static struct argp_option options[] = {
  {"buffer-size",       's',    "LEN",  0,      "Length in bytes of input buffer (10MB)"},
  {"fasta",             'f',    0,      0,      "Print k-mers in fasta format (false)"},
  {"column",            'c',    0,      0,      "Print k-mers in column format (false)"},
  {"recompute",         'r',    0,      0,      "Recompute statistics"},
  {"verbose",           'v',    0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  bool   fasta;
  bool   column;
  bool   verbose;
  bool   recompute;
  size_t buff_size;
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
  case 'f': FLAG(fasta);
  case 'c': FLAG(column);
  case 'r': FLAG(recompute);
  case 'v': FLAG(verbose);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int stats_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.buff_size = 10000000;
  arguments.fasta = false;
  arguments.column = false;
  arguments.verbose = false;
  arguments.recompute = false;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  hash_reader_t hash(argv[arg_st]);
  if(arguments.verbose) {
    std::cout << "k-mer length (bases):          " <<
      (hash.get_key_len() / 2) << std::endl;
    std::cout << "value length (bytes):  " <<
      hash.get_val_len() << std::endl;
  }

  if(!(arguments.fasta || arguments.column) && !arguments.recompute) {
    std::cout << 
      "Unique:    " << hash.get_unique() << "\n" <<
      "Distinct:  " << hash.get_distinct() << "\n" <<
      "Total:     " << hash.get_total() << "\n" <<
      "Max Count: " << hash.get_max_count() << std::endl;
    return 0;
  }

  if(arguments.fasta) {
    char key[hash.get_mer_len() + 1];
    while(hash.next()) {
      hash.get_string(key);
      std::cout << ">" << hash.val << "\n" << key << "\n";
    }
  } else if(arguments.column) {
    char key[hash.get_mer_len() + 1];
    while(hash.next()) {
      hash.get_string(key);
      std::cout << key << " " << hash.val << "\n";
    }
  } else {
    uint64_t unique = 0, distinct = 0, total = 0, max_count = 0;
    while(hash.next()) {
      unique += hash.val == 1;
      distinct++;
      total += hash.val;
      if(hash.val > max_count)
        max_count = hash.val;
    }
    std::cout << 
      "Unique:    " << unique << "\n" <<
      "Distinct:  " << distinct << "\n" <<
      "Total:     " << total << "\n" <<
      "Max_count: " << max_count << std::endl;
  }

  return 0;
}
