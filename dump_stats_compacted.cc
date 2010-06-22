#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>
#include <iostream>
#include <fstream>
#include "compacted_hash.hpp"

/*
 * Option parsing
 */
const char *argp_program_version = "dump_stats_compacted 1.0";
const char *argp_program_bug_address = "<guillaume@marcais.net>";
static char doc[] = "Dump k-mer statistics from a compacted database";
static char args_doc[] = "database";

static struct argp_option options[] = {
  {"buffer-size",       's',    "LEN",  0,      "Length in bytes of input buffer (10MB)"},
  {"fasta",             'f',    0,      0,      "Print k-mers in fasta format (false)"},
  {"recompute",         'r',    0,      0,      "Recompute statistics"},
  {"verbose",           'v',    0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  bool          fasta;
  bool          verbose;
  bool          recompute;
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
  case 'r': FLAG(recompute);
  case 'v': FLAG(verbose);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

struct header {
  uint64_t klen;
  uint64_t clen;
  uint64_t size;
};

struct trailer {
  uint64_t unique;
  uint64_t distinct;
  uint64_t total;
};

int main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.buff_size = 10000000;
  arguments.fasta = false;
  arguments.verbose = false;
  arguments.recompute = false;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 1) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  jellyfish::compacted_hash::reader hash(argv[arg_st]);

  if(arguments.verbose) {
    std::cout << "k-mer length:          " <<
      hash.get_mer_len() << std::endl;
    std::cout << "value length (bytes):  " <<
      hash.get_val_len() << std::endl;
  }

  std::cout << 
    "Unique:   " << hash.get_unique() << std::endl <<
    "Distinct: " << hash.get_distinct() << std::endl <<
    "Total:    " << hash.get_total() << std::endl;


  // Reimplement as an iterator in compacted_hash class.
//   if(arguments.fasta || arguments.recompute) {
//     char *buffer = new char[buffer_len];
//     char *buffer_end, *ptr;
//     uint64_t key, count, distinct = 0, unique = 0, total = 0;
//     char kmer[header.klen+1];
//     char table[4] = { 'A', 'C', 'G', 'T' };
//     kmer[header.klen] = '\0';
    
//     hash_c.seekg(-(sizeof(trailer) + record_len), std::ios_base::end);
//     std::streampos length = hash_c.tellg();
//     std::streampos cur = sizeof(header);

//     hash_c.seekg(sizeof(header), std::ios_base::beg);
//     while(hash_c.good()) {
//       hash_c.read(buffer, buffer_len);
//       //    std::cerr << "Read " << hash_c.gcount() << "\n";
//       buffer_end = buffer + hash_c.gcount();
//       ptr = buffer;
      
//       while(ptr + record_len <= buffer_end && cur <= length) {
//         key = 0;
//         memcpy(&key, ptr, klen);
//         ptr += klen;
//         count = 0;
//         memcpy(&count, ptr, header.clen);
//         ptr += header.clen;
//         cur += klen + header.clen;
        
//         if(arguments.fasta) {
//           for(unsigned int i = 0; i < header.klen; i++) {
//             kmer[header.klen - 1 - i] = table[key & 0x3];
//             key >>= 2;
//           }
//           std::cout << ">" << count << std::endl << kmer << std::endl;
//         }
        
//         if(count == 1)
//           unique++;
//         distinct++;
//         total += count;
//       }
//     }
//     hash_c.close();

//     if(arguments.recompute) {
//       std::cout << "Stats recomputed:" << std::endl <<
//         "Unique:   " << unique << std::endl <<
//         "Distinct: " << distinct << std::endl <<
//         "Total:    " << total << std::endl;
//     }
//   }

  return 0;
}
