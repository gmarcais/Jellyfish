#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>
#include <iostream>
#include <fstream>
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

#define ULONGP(field) errno = 0; \
arguments->field = (typeof(arguments->field))strtoul(arg,NULL,0);     \
if(errno) return errno; \
break;

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
      "Unique:   " << hash.get_unique() << std::endl <<
      "Distinct: " << hash.get_distinct() << std::endl <<
      "Total:    " << hash.get_total() << std::endl;
    return 0;
  }

  if(arguments.fasta) {
    char key[hash.get_mer_len() + 1];
    while(hash.next()) {
      hash.get_string(key);
      std::cout << ">" << hash.val << std::endl << key << std::endl;
    }
  } else if(arguments.column) {
    char key[hash.get_mer_len() + 1];
    while(hash.next()) {
      hash.get_string(key);
      std::cout << key << " " << hash.val << std::endl;
    }
  } else {
    uint64_t unique = 0, distinct = 0, total = 0;
    while(hash.next()) {
      unique += hash.val == 1;
      distinct++;
      total += hash.val;
    }
    std::cout << 
      "Unique:   " << unique << std::endl <<
      "Distinct: " << distinct << std::endl <<
      "Total:    " << total << std::endl;
  }

  return 0;
}
