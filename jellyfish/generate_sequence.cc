#include <argp.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>

#include <jellyfish/misc.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/randomc.h>

/*
 * Option parsing
 */
static char doc[] = "Generate random sequence";
static char args_doc[] = "seed";

static struct argp_option options[] = {
  {"mer",       'm',    "SIZE",         0,      "Mer size. Generate matrix of size 2*m"},
  {"output",    'o',    "PREFIX",       0,      "Output prefix"},
  {"verbose",           'v',            0,      0,      "Be verbose (false)"},
  { 0 }
};

struct arguments {
  std::vector<int>  sizes;
  const char       *output;
  bool verbose;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;
  unsigned long tmp;
  error_t error;

#define ULONGP(field) \
  error = parse_long(arg, &std::cerr, &arguments->field);       \
  if(error) return(error); else break;

#define AULONGP(field)                       \
  error = parse_long(arg, &std::cerr, &tmp); \
  if(error) return(error);                   \
  arguments->field.push_back(tmp);           \
  break;

#define FLAG(field) arguments->field = true; break;

#define STRING(field) arguments->field = arg; break;

  switch(key) {
  case 'm': AULONGP(sizes);
  case 'v': FLAG(verbose);
  case 'o': STRING(output);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

class rDNAg_t {
public:
  rDNAg_t(CRandomMersenne *_rng) : rng(_rng), i(15), buff(0) {}
  char letter() {
    i = (i+1) % 16;
    if(i == 0)
      buff = rng->BRandom();
    char res = letters[buff & 0x3];
    buff >>= 2;
    return res;
  }
private:
  CRandomMersenne *rng;
  int i;
  uint32_t buff;
  static const char letters[4];
};
const char rDNAg_t::letters[4] = { 'A', 'C', 'G', 'T' };

int main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.output = "output";
  arguments.verbose      = false;

  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(arg_st != argc - 2) {
    fprintf(stderr, "Wrong number of argument\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }
  unsigned long seed;
  error_t error = parse_long(argv[arg_st++], &std::cerr, &seed);
  if(error)
    return error;

  size_t length;
  error = parse_long(argv[arg_st], &std::cerr, &length);
  if(error)
    return error;
  

  if(arguments.verbose)
    std::cout << "Seed: " << seed << "\n";
  CRandomMersenne rng(seed);
  
  // Generate matrices
  uint64_t lines[64];
  for(std::vector<int>::iterator it = arguments.sizes.begin();
      it < arguments.sizes.end(); it++) {
    if(*it <= 0 || *it > 32)
      die("Invalid mer size '%d'. It must be between 1 and 32.", *it);
    int matrix_size = *it << 1;
    SquareBinaryMatrix mat(matrix_size), inv(matrix_size);
    while(true) {
      for(int i = 0; i < matrix_size; i++)
        lines[i] = (uint64_t)rng.BRandom() | ((uint64_t)rng.BRandom() << 32);
      mat = SquareBinaryMatrix(lines, matrix_size);
      try {
        inv = mat.inverse();
        break;
      } catch(SquareBinaryMatrix::SingularMatrix e) {}
    }
    char path[4096];
    int len = snprintf(path, sizeof(path), "%s_matrix_%d", arguments.output, *it);
    if(len < 0)
      die("Error creating the matrix file path: %s", strerror_string(errno).c_str());
    if((unsigned int)len >= sizeof(path))
      die("Output prefix too long '%s'", arguments.output);
    std::ofstream fd(path);
    if(!fd.good())
      die("Can't open matrix file '%s': %s", path, strerror_string(errno).c_str());
    if(arguments.verbose)
      std::cout << "Creating matrix file '" << path << "'\n";
    mat.dump(&fd);
    if(!fd.good())
      die("Error while writing matrix '%s': %s", path, strerror_string(errno).c_str());
    fd.close();
  }
  
  // Output sequence
  rDNAg_t rDNAg(&rng);
  char path[4096];
  int len = snprintf(path, sizeof(path), "%s.fa", arguments.output);
  if(len < 0)
    die("Error creating the fasta file path: %s", strerror_string(errno).c_str());
  if((unsigned int)len >= sizeof(path))
    die("Output prefix too long '%s'", arguments.output);
  std::ofstream fd(path);
  if(!fd.good())
    die("Can't open fasta file '%s': %s", path, strerror_string(errno).c_str());
  if(arguments.verbose)
    std::cout << "Creating fasta file '" << path << "'\n";
  fd << ">read\n";
  size_t total_len = 0;
  while(total_len < length) {
    for(int i = 0; i < 70 && total_len < length; i++) {
      fd << rDNAg.letter();
      total_len++;
    }
    fd << "\n";
  }
  if(!fd.good())
    die("Error while writing fasta file '%s': %s", path, strerror_string(errno).c_str());
  fd.close();
  return 0;
}
