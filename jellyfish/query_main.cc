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
static char doc[] = "Query from a compacted database";
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

#define ULONGP(field) errno = 0; \
arguments->field = (typeof(arguments->field))strtoul(arg,NULL,0);     \
if(errno) return errno; \
break;

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

int query_main(int argc, char *argv[])
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

  hash_query_t hash(argv[arg_st]);
  hash.set_canonical(arguments.both_strands);
  uint_t mer_len = hash.get_mer_len();
  const uint_t width = mer_len + 2;
  if(arguments.verbose) {
    std::cout << "k-mer length (bases):          " <<
      (hash.get_key_len() / 2) << "\n";
    std::cout << "value length (bytes):  " <<
      hash.get_val_len() << "\n";
  }

  char mer[width + 2];
  while(true) {
    std::cin >> std::setw(width) >> mer;
    if(!std::cin.good())
      break;
    if(strlen(mer) != mer_len) {
      die("Invalid mer %s\n", mer);
    }
    std::cout << mer << " " << hash[mer] << "\n";
  }

  return 0;
}
