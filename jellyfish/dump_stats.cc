#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#include "mer_counting.hpp"
#include "lib/misc.hpp"

/*
 * Option parsing
 */
const char *argp_program_version = "dump_stats_compacted 1.0";
const char *argp_program_bug_address = "<guillaume@marcais.net>";
static char doc[] = "Dump k-mer statistics";
static char args_doc[] = "database";

static struct argp_option options[] = {
  {"fasta",             'f',    0,      0,      "Print k-mers in fasta format (false)"},
  {"verbose",           'v',    0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  bool          fasta;
  bool          verbose;
  size_t        buff_size;
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
  case 'v': FLAG(verbose);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[]) {
  struct arguments  arguments;
  int               arg_st;
  const char       *db_file;
  uint64_t          unique_mers   = 0;
  uint64_t          total_mers    = 0;
  uint64_t          distinct_mers = 0;
  //  off_t expect_size;

  arguments.fasta = false;
  arguments.verbose = false;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  db_file = argv[arg_st];
  mer_counters hash_table(db_file, true);
  mer_counters::iterator it = hash_table.iterator_all();

  unsigned int rklen = hash_table.get_key_len() / 2;
  char kmer[rklen+1];
  kmer[rklen] = '\0';

  while(it.next()) {
    if(arguments.fasta) {
      it.get_string(kmer);
      std::cout << ">" << it.val << std::endl << kmer << std::endl;
    }
    
    distinct_mers++;
    if(it.val == 1)
      unique_mers++;
    total_mers += it.val;
  }

  if(!arguments.fasta)
    std::cout << std::dec << unique_mers << " " << distinct_mers << 
      " " << total_mers << std::endl;
  
  exit(0);
}
