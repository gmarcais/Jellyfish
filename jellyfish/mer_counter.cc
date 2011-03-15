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
#include <pthread.h>
#include <fstream>
#include <exception>
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
#include <inttypes.h>

#include <jellyfish/misc.hpp>
#include <jellyfish/time.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/fasta_parser.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/square_binary_matrix.hpp>

// Temporary
//#include <jellyfish/measure_dumper.hpp>

/*
 * Option parsing
 */
static char doc[] = "Count k-mers in a fasta or fastq files.\n"
  "The length k of the mer (-m) and the size of the hash (-s) must be given.";
static char args_doc[] = "fasta|fastq ...";

struct arguments {
#define DFT_NB_THREADS  1
  unsigned long	 nb_threads;
#define DFT_BUFFER_SIZE 4096
  unsigned long	 buffer_size;
#define DFT_NB_BUFFERS 0
  unsigned long	 nb_buffers;
#define DFT_MER_LEN 0
  unsigned long	 mer_len;
#define DFT_COUNTER_LEN 7
  unsigned long	 counter_len;
#define DFT_OUT_COUNTER_LEN 4
  unsigned long	 out_counter_len;
#define DFT_REPROBES 50
  unsigned long  reprobes;
#define DFT_SIZE 0
  unsigned long	 size;
#define DFT_OUT_BUFFER_SIZE 20000000UL
  unsigned long	 out_buffer_size;
#define DFT_NO_WRITE false
  bool           no_write;
#define DFT_BOTH_STRANDS false
  bool           both_strands;
#define DFT_RAW false
  bool           raw;
#define DFT_MEASURE false
  bool           measure;
#define DFT_TIMING 0
  char          *timing;
#define DFT_OUTPUT "mer_counts.hash"
  const char    *output;
#define DFT_MATRIX 0
  char          *matrix;
#define DFT_FASTQ false
  bool           fastq;
#define DFT_QUALITY_CONTROL false
  bool           quality_control;
#define DFT_QUALITY_START 64
  unsigned int   quality_start;

  arguments() :
    nb_threads(DFT_NB_THREADS),
    buffer_size(DFT_BUFFER_SIZE),
    nb_buffers(DFT_NB_BUFFERS),
    mer_len(DFT_MER_LEN),
    counter_len(DFT_COUNTER_LEN),
    out_counter_len(DFT_OUT_COUNTER_LEN),
    reprobes(DFT_REPROBES),
    size(DFT_SIZE),
    out_buffer_size(DFT_OUT_BUFFER_SIZE),
    no_write(DFT_NO_WRITE),
    both_strands(DFT_BOTH_STRANDS),
    raw(DFT_RAW),
    measure(DFT_MEASURE),
    timing(DFT_TIMING),
    output(DFT_OUTPUT),
    matrix(DFT_MATRIX),
    fastq(DFT_FASTQ),
    quality_control(DFT_QUALITY_CONTROL),
    quality_start(DFT_QUALITY_START) {}

  void check(struct argp_state *argp) {
    if(mer_len == 0)
      argp_error(argp, "Specifying the mer size (-m) is required");
    if(size == 0)
      argp_error(argp, "Spcifying the hash table size (-s) is required.\nSee the man page for how to estimate this parameter.");
    if(nb_buffers == 0)
      nb_buffers = 3 * nb_threads;
  }
};
     
enum {
  OPT_VAL_LEN = 1024,
  OPT_BUF_SIZE,
  OPT_TIMING,
  OPT_OBUF_SIZE,
  OPT_MATRIX,
  OPT_QUAL_CON,
  OPT_QUAL_START
};
static struct argp_option options[] = {
  {"threads",           't',            "NB",   0,              "Nb of threads" DFTS(DFT_NB_THREADS), 1},
  {"mer-len",           'm',            "LEN",  0,              "Length of a mer", 0},
  {"counter-len",       'c',            "LEN",  0,              "Length (in bits) of counting field" DFTS(DFT_COUNTER_LEN), 0},
  {"output",            'o',            "FILE", 0,              "Output file" DFTS(DFT_OUTPUT), 1},
  {"out-counter-len",   OPT_VAL_LEN,    "LEN",  0,              "Length (in bytes) of counting field in output" DFTS(DFT_OUT_COUNTER_LEN), 1},
  {"fastq",             'q',            0,      0,              "Fastq input files", 2},
  {"quality-control",   OPT_QUAL_CON,   0,      0,              "B quality in fastq is Read Segment Quality Control Indicator", 2},
  //  {"quality-start",     OPT_QUAL_START, VAL     0,              "Starting ASCII for quality values", 2},
  {"both-strands",      'C',            0,      0,              "Count both strands, canonical representation", 1},
  {"buffer-size",       OPT_BUF_SIZE,   "SIZE", OPTION_HIDDEN,  "Size of a buffer" DFTS(DFT_BUFFER_SIZE)},
  {"out-buffer-size",   OPT_OBUF_SIZE,  "SIZE", OPTION_HIDDEN,  "Size of output buffer per thread" DFTS(DFT_OUT_BUFFER_SIZE)},
  {"buffers",           'b',            "NB",   OPTION_HIDDEN,  "Nb of buffers per thread"},
  {"hash-size",         's',            "SIZE", 0,              "Hash size", 0},
  {"size",              's',            "SIZE", OPTION_ALIAS,   "Hash size"},
  {"reprobes",          'p',            "NB",   0,              "Maximum number of reprobing" DFTS(DFT_REPROBES), 1},
  {"no-write",          'w',            0,      0,              "Don't write hash to disk", 3},
  {"raw",               'r',            0,      0,              "Dump raw database", 2},
  {"measure",           'u',            0,      OPTION_HIDDEN,  "Dump usage statistics", 2},
  {"matrix",            OPT_MATRIX,     "FILE", 0,              "Matrix for hash function", 3},
  {"timing",            OPT_TIMING,     "FILE", 0,              "Print timing information to FILE", 3},
  { 0 }
};


static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;
  error_t error;

  switch(key) {
    PARSE_ARG_NUM('t', nb_threads);
    PARSE_ARG_NUM('s', size);
    PARSE_ARG_NUM('b', nb_buffers);
    PARSE_ARG_NUM('m', mer_len);
    PARSE_ARG_NUM('c', counter_len);
    PARSE_ARG_NUM('p', reprobes);
    PARSE_ARG_FLAG('w', no_write);
    PARSE_ARG_FLAG('r', raw);
    PARSE_ARG_FLAG('u', measure);
    PARSE_ARG_FLAG('q', fastq);
    PARSE_ARG_FLAG('C', both_strands);
    PARSE_ARG_STRING('o', output);
    PARSE_ARG_NUM(OPT_VAL_LEN, out_counter_len);
    PARSE_ARG_NUM(OPT_BUF_SIZE, buffer_size);
    PARSE_ARG_STRING(OPT_TIMING, timing);
    PARSE_ARG_NUM(OPT_OBUF_SIZE, out_buffer_size);
    PARSE_ARG_STRING(OPT_MATRIX, matrix);
    PARSE_ARG_FLAG(OPT_QUAL_CON, quality_control);

  case ARGP_KEY_SUCCESS:
    arguments->check(state);
    break;

  case ARGP_KEY_NO_ARGS:
    argp_error(state, "Missing input sequence file(s)");
    
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

class mer_counting_base {
public:
  virtual void count() = 0;
  virtual Time get_writing_time() = 0;
  virtual ~mer_counting_base() {}
};

template <typename parser_t, typename hash_t>
class mer_counting : public mer_counting_base, public thread_exec {
protected:
  struct arguments            arguments;
  locks::pthread::barrier     sync_barrier;
  parser_t                   *parser;
  typename hash_t::storage_t *ary;
  hash_t                     *hash;
  jellyfish::dumper_t        *dumper;

public:
  mer_counting(struct arguments &args) :
    arguments(args), sync_barrier(arguments.nb_threads) {}

  ~mer_counting() { 
    if(dumper)
      delete dumper;
    if(hash)
      delete hash;
    if(ary)
      delete ary;
    if(parser)
      delete parser;
  }
  
  void start(int id) {
    sync_barrier.wait();
    
    typename parser_t::thread     mer_stream(parser->new_thread());
    typename hash_t::thread_ptr_t counter(hash->new_thread());
    mer_stream.parse(counter);
    
    bool is_serial = sync_barrier.wait() == PTHREAD_BARRIER_SERIAL_THREAD;
    if(is_serial)
      hash->dump();
  }
  
  void count() {
    exec_join(arguments.nb_threads);
  }

  Time get_writing_time() { return hash->get_writing_time(); }
};


class mer_counting_fasta_hash : public mer_counting<jellyfish::parse_dna, inv_hash_t> {
public:
  mer_counting_fasta_hash(int arg_st, int argc, char *argv[], struct arguments &_args) :
    mer_counting<jellyfish::parse_dna, inv_hash_t>(_args)
  {
    parser = new jellyfish::parse_dna(argc - arg_st, argv + arg_st, 
                                      arguments.mer_len, arguments.nb_buffers,
                                      arguments.buffer_size);
    ary = new inv_hash_t::storage_t(arguments.size, 2*arguments.mer_len,
                                    arguments.counter_len, 
                                    arguments.reprobes, 
                                    jellyfish::quadratic_reprobes);
    if(_args.matrix) {
      std::ifstream fd;
      fd.exceptions(std::ifstream::eofbit|std::ifstream::failbit|std::ifstream::badbit);
      fd.open(_args.matrix);
      SquareBinaryMatrix m(&fd);
      fd.close();
      ary->set_matrix(m);
    }
    hash = new inv_hash_t(ary);

    if(!arguments.no_write) {
      // if(arguments.measure) {
      //   dumper = new jellyfish::measure_dumper<inv_hash_t::storage_t>(ary);
      // } else
      if(arguments.raw) {
        dumper = new raw_inv_hash_dumper_t((uint_t)4, arguments.output, arguments.out_buffer_size, ary);
      } else {
        dumper = new inv_hash_dumper_t(arguments.nb_threads, arguments.output,
                                       arguments.out_buffer_size,
                                       8*arguments.out_counter_len,
                                       ary);
      }
      hash->set_dumper(dumper);
    }
    parser->set_canonical(arguments.both_strands);
  }
};

class mer_counting_fasta_direct : public mer_counting<jellyfish::parse_dna, direct_index_t> {
public:
  mer_counting_fasta_direct(int arg_st, int argc, char *argv[], struct arguments &_args) :
    mer_counting<jellyfish::parse_dna, direct_index_t>(_args)
  {
    parser = new jellyfish::parse_dna(argc - arg_st, argv + arg_st, 
                                      arguments.mer_len, arguments.nb_buffers,
                                      arguments.buffer_size);
    ary = new direct_index_t::storage_t(2 * arguments.mer_len);
    hash = new direct_index_t(ary);
    if(!arguments.no_write) {
      if(arguments.raw)
        std::cerr << "Switch --raw not (yet) supported with direct indexing. Ignoring." << std::endl;
      dumper = new direct_index_dumper_t(arguments.nb_threads, arguments.output,
                                         arguments.out_buffer_size,
                                         8*arguments.out_counter_len,
                                         ary);
      hash->set_dumper(dumper);
    }
    parser->set_canonical(arguments.both_strands);
  }
};

// class mer_counting_fastq : public mer_counting<jellyfish::fastq_parser, fastq_hash_t> {
// public:
//   mer_counting_fastq(int arg_st, int argc, char *argv[], struct arguments &_args) :
//     mer_counting<jellyfish::fastq_parser, fastq_hash_t>(_args)
//   {
//     parser = new jellyfish::fastq_parser(argc - arg_st, argv + arg_st,
//                                          arguments.mer_len, arguments.nb_buffers, 
//                                          arguments.quality_control, arguments.quality_start);
//     ary = new fastq_hash_t::storage_t(arguments.size, 2*arguments.mer_len,
//                                       arguments.reprobes, 
//                                       jellyfish::quadratic_reprobes);
//     hash = new fastq_hash_t(ary);
//     if(!arguments.no_write) {
//       dumper = new raw_fastq_dumper_t(arguments.nb_threads, arguments.output,
//                                       arguments.out_buffer_size,
//                                       ary);
//       hash->set_dumper(dumper);
//     }
//     parser->set_canonical(arguments.both_strands);
//   }
// };

int count_main(int argc, char *argv[]) {
  struct arguments arguments;
  int arg_st;

  int error = argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);
  if(error)
    die("Error parsing arguments. Try --help: %s", strerror(error));

  Time start;
  mer_counting_base *counter;
  // if(arguments.fastq) {
  //   counter = new mer_counting_fastq(arg_st, argc, argv, arguments);
  // } else
  if(ceilLog2(arguments.size) > 2 * arguments.mer_len) {
    counter = new mer_counting_fasta_direct(arg_st, argc, argv, arguments);
  } else {
    counter = new mer_counting_fasta_hash(arg_st, argc, argv, arguments);
  }
  Time after_init;
  counter->count();
  Time all_done;

  if(arguments.timing) {
    std::ofstream timing_fd(arguments.timing);
    if(!timing_fd.good()) {
      fprintf(stderr, "Can't open timing file '%s': %s\n",
              arguments.timing, strerror(errno));
    } else {
      Time writing = counter->get_writing_time();
      Time counting = (all_done - after_init) - writing;
      timing_fd << "Init     " << (after_init - start).str() << "\n";
      timing_fd << "Counting " << counting.str() << "\n";
      timing_fd << "Writing  " << writing.str() << std::endl;
      timing_fd.close();
    }
  }

  return 0;
}
