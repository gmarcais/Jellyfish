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
static char doc[] = "Query from a raw hash database";
static char args_doc[] = "database";

enum {
  OPT_BOTH = 1024
};

static struct argp_option options[] = {
  {"both-strands",      OPT_BOTH,       0,      0,      "Both strands"},
  {"verbose",           'v',            0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  bool both_strands;
  bool verbose;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;
  //  error_t error;

#define ULONGP(field) \
  error = parse_long(arg, &std::cerr, &arguments->field);       \
  if(error) return(error); else break;

#define FLAG(field) arguments->field = true; break;

  switch(key) {
  case OPT_BOTH: FLAG(both_strands);
  case 'v': FLAG(verbose);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int raw_query_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.both_strands = false;
  arguments.verbose      = false;

  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  mapped_file dbf(argv[arg_st]);
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(strncmp(type, "JFRHSHDN", sizeof(8)))
    die("Invalid database type '%.8s', expected '%.8s'",
        dbf.base(), "JFRHSHDN");
  inv_hash_storage_t hash(dbf.base() + 8, dbf.length() - 8);

  uint_t mer_len = hash.get_key_len() / 2;
  const uint_t width = mer_len + 2;
  if(arguments.verbose) {
    std::cout << "k-mer length (bases): " <<
      (hash.get_key_len() / 2) << "\n";
    std::cout << "value length (bits):  " <<
      hash.get_val_len() << "\n";
  }

  char mer[width + 2];
  char mer2[width + 2];
  uint64_t key;
  uint64_t val;
  while(true) {
    std::cin >> std::setw(width) >> mer;
    if(!std::cin.good())
      break;
    if(strlen(mer) != mer_len) {
      die("Invalid mer %s\n", mer);
    }
    key = jellyfish::fasta_parser::mer_string_to_binary(mer, mer_len);
    if(arguments.both_strands) {
      uint64_t rev = jellyfish::fasta_parser::reverse_complement(key, mer_len);
      if(rev < key)
        key = rev;
    }
    if(!hash.get_val(key, val, true))
      val = 0;
    jellyfish::fasta_parser::mer_binary_to_string(key, mer_len, mer2);
    std::cout << mer << " " << mer2 << " " << val << "\n";
  }

  return 0;
}
