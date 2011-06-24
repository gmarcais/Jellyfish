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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <inttypes.h>

#include <jellyfish/err.hpp>
#include <jellyfish/dbg.hpp>
#include <jellyfish/backtrace.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/time.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/mer_counter_cmdline.hpp>
#include <jellyfish/noop_dumper.hpp>

// Temporary
//#include <jellyfish/measure_dumper.hpp>

// struct mer_counter_args {
//   int    mer_len_given;
//   int    mer_len_arg;
//   int    size_given;
//   long   size_arg;
//   int    threads_given;
//   int    threads_arg;
//   int    output_given;
//   char  *output_arg;
//   int    counter_len_given;
//   int    counter_len_arg;
//   int    out_counter_len_given;
//   int    out_counter_len_arg;
//   int    both_strands_flag;
//   int    reprobes_given;
//   int    reprobes_arg;
//   int    raw_flag;
//   int    both_flag;
//   int    quake_flag;
//   int    quality_start_given;
//   int    quality_start_arg;
//   int    min_quality_given;
//   int    min_quality_arg;
//   int    lower_count_given;
//   long   lower_count_arg;
//   int    upper_count_given;
//   long   upper_count_arg;
//   int    matrix_given;
//   char  *matrix_arg;
//   int    timing_given;
//   char  *timing_arg;
//   int    stats_given;
//   int    no_write_flag;
//   char  *stats_arg;
//   int    measure_flag;
//   int    buffers_given;
//   long   buffers_arg;
//   int    buffer_size_given;
//   long   buffer_size_arg;
//   int    out_buffer_size_given;
//   long   out_buffer_size_arg;
//   int    lock_flag;
//   int    stream_flag;
//   int    inputs_num;
//   char **inputs;
// };


// TODO: This mer_counting_base stuff has become wild. Lots of code
// duplication and slightly different behavior for each (e.g. setup of
// the hashing matrix). Refactor!
class mer_counting_base {
public:
  virtual ~mer_counting_base() {}
  virtual void count() = 0;
  virtual Time get_writing_time() const = 0;
  virtual uint64_t get_distinct() const = 0;
  virtual uint64_t get_total() const = 0;
};

template <typename parser_t, typename hash_t>
class mer_counting : public mer_counting_base, public thread_exec {
protected:
  struct mer_counter_args    *args;
  locks::pthread::barrier     sync_barrier;
  parser_t                   *parser;
  typename hash_t::storage_t *ary;
  hash_t                     *hash;
  jellyfish::dumper_t        *dumper;
  uint64_t                    distinct, total;

public:
  mer_counting(struct mer_counter_args &_args) :
    args(&_args), sync_barrier(args->threads_arg),
    distinct(0), total(0) {}

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
    if(is_serial) {
      hash->dump();
    }

    atomic::gcc::fetch_add(&distinct, mer_stream.get_distinct());
    atomic::gcc::fetch_add(&total, mer_stream.get_total());
  }
  
  void count() {
    exec_join(args->threads_arg);
  }

  virtual Time get_writing_time() const { return hash->get_writing_time(); }
  virtual uint64_t get_distinct() const { return distinct; }
  virtual uint64_t get_total() const { return total; }
};

class mer_counting_fasta_hash : public mer_counting<jellyfish::parse_dna, inv_hash_t> {
public:
  mer_counting_fasta_hash(int argc, const char *argv[], 
                          struct mer_counter_args &_args) :
    mer_counting<jellyfish::parse_dna, inv_hash_t>(_args)
  {
    parser = new jellyfish::parse_dna(argc, argv, 
                                      args->mer_len_arg, args->buffers_arg,
                                      args->buffer_size_arg);
    ary = new inv_hash_t::storage_t(args->size_arg, 2*args->mer_len_arg,
                                    args->counter_len_arg, 
                                    args->reprobes_arg, 
                                    jellyfish::quadratic_reprobes);
    if(args->matrix_given) {
      std::ifstream fd;
      fd.exceptions(std::ifstream::eofbit|std::ifstream::failbit|std::ifstream::badbit);
      fd.open(args->matrix_arg);
      SquareBinaryMatrix m(&fd);
      fd.close();
      ary->set_matrix(m);
    }
    hash = new inv_hash_t(ary);

    if(args->no_write_flag) {
      dumper = new jellyfish::noop_dumper();
    } else {
      // if(args->measure) {
      //   dumper = new jellyfish::measure_dumper<inv_hash_t::storage_t>(ary);
      // } else
      if(args->raw_flag) {
        dumper = new raw_inv_hash_dumper_t((uint_t)4, args->output_arg,
                                           args->out_buffer_size_arg, ary);
      } else {
        inv_hash_dumper_t *_dumper =
          new inv_hash_dumper_t(args->threads_arg, args->output_arg,
                                args->out_buffer_size_arg, 
                                8*args->out_counter_len_arg, ary);
        if(args->lower_count_given)
          _dumper->set_lower_count(args->lower_count_arg);
        if(args->upper_count_given)
          _dumper->set_upper_count(args->upper_count_arg);
        dumper = _dumper;
      }
    }
    hash->set_dumper(dumper);
    parser->set_canonical(args->both_strands_flag);
  }
};

class mer_counting_qual_fasta_hash : public mer_counting<jellyfish::parse_qual_dna, inv_hash_t> {
public:
  mer_counting_qual_fasta_hash(int argc, char *argv[], 
                               struct mer_counter_args &_args) :
    mer_counting<jellyfish::parse_qual_dna, inv_hash_t>(_args)
  {
    parser = new jellyfish::parse_qual_dna(argc, argv, 
                                           args->mer_len_arg, args->buffers_arg,
                                           args->buffer_size_arg, args->quality_start_arg,
                                           args->min_quality_arg);
    ary = new inv_hash_t::storage_t(args->size_arg, 2*args->mer_len_arg,
                                    args->counter_len_arg, 
                                    args->reprobes_arg, 
                                    jellyfish::quadratic_reprobes);
    if(args->matrix_given) {
      std::ifstream fd;
      fd.exceptions(std::ifstream::eofbit|std::ifstream::failbit|std::ifstream::badbit);
      fd.open(args->matrix_arg);
      SquareBinaryMatrix m(&fd);
      fd.close();
      ary->set_matrix(m);
    }
    hash = new inv_hash_t(ary);

    if(args->no_write_flag) {
      dumper = new jellyfish::noop_dumper();
    } else {
      if(args->raw_flag) {
        dumper = new raw_inv_hash_dumper_t((uint_t)4, args->output_arg,
                                           args->out_buffer_size_arg, ary);
      } else {
        inv_hash_dumper_t *_dumper =
          new inv_hash_dumper_t(args->threads_arg, args->output_arg,
                                args->out_buffer_size_arg, 
                                8*args->out_counter_len_arg, ary);
        if(args->lower_count_given)
          _dumper->set_lower_count(args->lower_count_arg);
        if(args->upper_count_given)
          _dumper->set_upper_count(args->upper_count_arg);
        dumper = _dumper;
      }
    }
    hash->set_dumper(dumper);
    parser->set_canonical(args->both_strands_flag);
  }
};


class mer_counting_fasta_direct : public mer_counting<jellyfish::parse_dna, direct_index_t> {
public:
  mer_counting_fasta_direct(int argc, const char *argv[], 
                            struct mer_counter_args &_args) :
    mer_counting<jellyfish::parse_dna, direct_index_t>(_args)
  {
    parser = new jellyfish::parse_dna(argc, argv, 
                                      args->mer_len_arg, args->buffers_arg,
                                      args->buffer_size_arg);
    ary = new direct_index_t::storage_t(2 * args->mer_len_arg);
    hash = new direct_index_t(ary);
    if(args->no_write_flag) {
      dumper = new jellyfish::noop_dumper();
    } else {
      if(args->raw_flag)
        std::cerr << "Switch --raw not (yet) supported with direct indexing. Ignoring." << std::endl;
      dumper = new direct_index_dumper_t(args->threads_arg, args->output_arg,
                                         args->out_buffer_size_arg,
                                         8*args->out_counter_len_arg,
                                         ary);
    }
    hash->set_dumper(dumper);
    parser->set_canonical(args->both_strands_flag);
  }
};

class mer_counting_quake : public mer_counting<jellyfish::parse_quake, fastq_hash_t> {
public:
  mer_counting_quake(int argc, char *argv[],
                     struct mer_counter_args &_args) :
    mer_counting<jellyfish::parse_quake, fastq_hash_t>(_args)
  {
    parser = new jellyfish::parse_quake(argc, argv,
                                        args->mer_len_arg, args->buffers_arg, 
                                        args->buffer_size_arg, 
                                        args->quality_start_arg);
    ary = new fastq_hash_t::storage_t(args->size_arg, 2*args->mer_len_arg,
                                      args->reprobes_arg, 
                                      jellyfish::quadratic_reprobes);
    hash = new fastq_hash_t(ary);
    if(args->no_write_flag) {
      dumper = new jellyfish::noop_dumper();
    } else {
      dumper = new raw_fastq_dumper_t(args->threads_arg, args->output_arg,
                                      args->out_buffer_size_arg,
                                      ary);
    }
    hash->set_dumper(dumper);
    parser->set_canonical(args->both_strands_flag);
  }
};


void display_args(struct mer_counter_args &args) {
  if(!args.matrix_arg) args.matrix_arg = (char*)"";
  DBG << V(args.mer_len_given) << V(args.mer_len_arg) << "\n"
      << V(args.size_given) << V(args.size_arg) << "\n"
      << V(args.threads_given) << V(args.threads_arg) << "\n"
      << V(args.output_given) << V(args.output_arg) << "\n"
      << V(args.counter_len_given) << V(args.counter_len_arg) << "\n"
      << V(args.out_counter_len_given) << V(args.out_counter_len_arg) << "\n"
      << V(args.both_strands_flag) << "\n"
      << V(args.reprobes_given) << V(args.reprobes_arg) << "\n"
      << V(args.raw_flag) << "\n"
      << V(args.both_flag) << "\n"
      << V(args.quake_flag) << "\n"
      << V(args.quality_start_given) << V(args.quality_start_arg) << "\n"
      << V(args.min_quality_given) << V(args.min_quality_arg) << "\n"
      << V(args.lower_count_given) << V(args.lower_count_arg) << "\n"
      << V(args.upper_count_given) << V(args.upper_count_arg) << "\n"
      << V(args.matrix_given) << V(args.matrix_arg) << "\n"
      << V(args.timing_given) << V(args.timing_arg) << "\n"
      << V(args.stats_given) << V(args.stats_arg) << "\n"
      << V(args.no_write_flag) << "\n"
      << V(args.measure_flag) << "\n"
      << V(args.buffers_given) << V(args.buffers_arg) << "\n"
      << V(args.buffer_size_given) << V(args.buffer_size_arg) << "\n"
      << V(args.out_buffer_size_given) << V(args.out_buffer_size_arg) << "\n"
      << V(args.lock_flag) << "\n"
      << V(args.stream_flag) << "\n";
}

int count_main(int argc, char *argv[])
{
  struct mer_counter_args args;

  if(mer_counter_cmdline(argc, argv, &args) != 0)
    die << "Command line parser failed";

  // args.mer_len_given         = 1;
  // args.mer_len_arg           = 16;
  // args.size_given            = 1;
  // args.size_arg              = 4000000000L;
  // args.threads_given         = 1;
  // args.threads_arg           = 27;
  // args.output_given          = 0;
  // args.output_arg            = (char *)"mers";
  // args.counter_len_given     = 1;
  // args.counter_len_arg       = 4;
  // args.out_counter_len_given = 1;
  // args.out_counter_len_arg   = 2;
  // args.both_strands_flag     = 1;
  // args.reprobes_given        = 1;
  // args.reprobes_arg          = 253;
  // args.raw_flag              = 0;
  // args.both_flag             = 0;
  // args.quake_flag            = 0;
  // args.quality_start_given   = 0;
  // args.quality_start_arg     = 33;
  // args.min_quality_given     = 0;
  // args.min_quality_arg       = 2;
  // args.lower_count_given     = 0;
  // args.lower_count_arg       = 0L;
  // args.upper_count_given     = 0;
  // args.upper_count_arg       = 10000L;
  // args.matrix_given          = 0;
  // args.matrix_arg            = 0;
  // args.timing_given          = 1;
  // args.timing_arg            = (char *)"timing";
  // args.stats_given           = 1;
  // args.stats_arg             = (char *)"stats";
  // args.no_write_flag         = 0;
  // args.measure_flag          = 0;
  // args.buffers_given         = 1;
  // args.buffers_arg           = 1024;
  // args.buffer_size_given     = 0;
  // args.buffer_size_arg       = 1024 * 1024;
  // args.out_buffer_size_given = 0;
  // args.out_buffer_size_arg   = 10 * 1024 * 1024;
  // args.lock_flag             = 0;
  // args.stream_flag           = 0;
  // args.inputs_num            = 1;
  // args.inputs                = (char **)malloc(sizeof(char*) * 2);
  // args.inputs[0]             = (char *)"/dev/fd/0";
  // args.inputs[1]             = 0;

  if(args.mer_len_arg < 2 || args.mer_len_arg > 31)
    die << "Invalid mer length '" << args.mer_len_arg
        << "'. It must be in [2, 31].";

  if(args.inputs_num == 0)
    die << "Need at least one input file or stream";

  display_args(args);

  show_backtrace();
  //  dbg::print_t::print_tid((pid_t)-1);
  //  dbg::print_t::set_signal();

  Time start;
  mer_counting_base *counter;
  if(!args.buffers_given)
    args.buffers_arg = 3 * args.threads_arg;
  DBG << V(args.buffers_arg);
  if(args.quake_flag) {
    counter = new mer_counting_quake(args.inputs_num, args.inputs, args);
  } else if(ceilLog2((unsigned long)args.size_arg) > 2 * (unsigned long)args.mer_len_arg) {
    counter = new mer_counting_fasta_direct(args.inputs_num, 
                                            (const char **)args.inputs, args);
  } else if(args.min_quality_given) {
    counter = new mer_counting_qual_fasta_hash(args.inputs_num, 
                                               args.inputs,
                                               args);
  } else {
    counter = new mer_counting_fasta_hash(args.inputs_num,
                                          (const char **)args.inputs, args);
  }
  Time after_init;
  counter->count();
  Time all_done;

  if(args.timing_given) {
    std::ofstream timing_fd(args.timing_arg);
    if(!timing_fd.good()) {
      std::cerr << "Can't open timing file '" << args.timing_arg << err::no
                << std::endl;
    } else {
      Time writing = counter->get_writing_time();
      Time counting = (all_done - after_init) - writing;
      timing_fd << "Init     " << (after_init - start).str() << "\n"
                << "Counting " << counting.str() << "\n"
                << "Writing  " << writing.str() << "\n";
      timing_fd.close();
    }
  }

  if(args.stats_given) {
    std::ofstream stats_fd(args.stats_arg);
    if(!stats_fd.good()) {
      std::cerr << "Can't open stats file '" << args.stats_arg << err::no
                << std::endl;
    } else {
      stats_fd << "Distinct: " << counter->get_distinct() << "\n"
               << "Total:    " << counter->get_total() << std::endl;
      stats_fd.close();
    }
  }

  return 0;
}
