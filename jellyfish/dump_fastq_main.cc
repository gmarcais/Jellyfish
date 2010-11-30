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
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/fastq_dumper.hpp>

/*
 * Option parsing
 */
static char doc[] = "Dump k-mer from a raw hash fastq database";
static char args_doc[] = "database";

static struct argp_option options[] = {
  {"column",    'c',    0,      0,      "Print k-mers in column format (false)"},
  {"verbose",   'v',    0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  bool   column;
  bool   verbose;
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
  case 'c': FLAG(column);
  case 'v': FLAG(verbose);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int dump_fastq_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.column    = false;
  arguments.verbose   = false;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  fastq_storage_t *hash = raw_fastq_dumper_t::read(argv[arg_st]);
  if(arguments.verbose) {
    std::cout << 
      "k-mer length (bases): " << (hash->get_key_len() / 2) << 
      "\nentries             : " << hash->get_size() <<
      "\n";
  }

  fastq_storage_t::iterator it = hash->iterator_all();
  std::cout << std::scientific;
  if(arguments.column) {
    while(it.next())
      std::cout << it.get_dna_str() << " " << it.get_val().to_float() << "\n";
  } else {
    while(it.next())
      std::cout << ">" << it.get_val().to_float() << "\n" << it.get_dna_str() << "\n";
  }

  return 0;
}
