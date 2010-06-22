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
  struct arguments arguments;
  int arg_st;
  int            fd;
  struct stat    stat;
  struct header *header;
  char          *db_file, *map;
  uint64_t       unique_mers = 0;
  uint64_t       total_mers  = 0;
  uint64_t       distinct_mers = 0;
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
  fd = open(db_file, O_RDONLY);
  if(fd < 0)
    die("Can't open '%s'\n", db_file);

  if(fstat(fd, &stat) < 0)
    die("Can't stat '%s'\n", db_file);

  map = (char *)mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if(map == MAP_FAILED)
    die("Can't mmap '%s'\n", db_file);
  close(fd);
  header = (struct header *)map;

  //  fprintf(stderr, "%ld %ld\n", header->klen, header->size);
//   expect_size = sizeof(struct header) + 
//     (sizeof(uint64_t) + sizeof(uint32_t)) * header->size;
//   if(stat.st_size != expect_size)
//     die("Size of '%s' is wrong. Expect %ld, got %ld\n", db_file, expect_size,
//         stat.st_size);

  mer_counters hash_table(map, stat.st_size);
  mer_counters::iterator it = hash_table.iterator_all();

  char table[4] = { 'A', 'C', 'G', 'T' };
  unsigned int rklen = hash_table.get_key_len();
  char kmer[rklen+1];
  kmer[rklen] = '\0';
  uint64_t key;

  while(it.next()) {
    if(arguments.fasta) {
      key = it.key;
      for(unsigned int i = 0; i < rklen; i++) {
        kmer[rklen - 1 - i] = table[key & UINT64_C(0x3)];
        key >>= 2;
      }
      std::cout << ">" << it.val << std::endl << kmer << std::endl;
    }
    
    distinct_mers++;
    if(it.val == 1)
      unique_mers++;
    total_mers += it.val;

    
  }

  std::cout << std::dec << unique_mers << " " << distinct_mers << " " << total_mers << std::endl;
  
  exit(0);
}
