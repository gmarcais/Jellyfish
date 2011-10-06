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
#include <jellyfish/count_main_cmdline.hpp>
#include <jellyfish/noop_dumper.hpp>

// Temporary
//#include <jellyfish/measure_dumper.hpp>

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
  count_args                 *args;
  locks::pthread::barrier     sync_barrier;
  parser_t                   *parser;
  typename hash_t::storage_t *ary;
  hash_t                     *hash;
  jellyfish::dumper_t        *dumper;
  uint64_t                    distinct, total;

public:
  mer_counting(count_args &_args) :
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
  mer_counting_fasta_hash(const std::vector<const char *> &files,
                          count_args &_args) :
    mer_counting<jellyfish::parse_dna, inv_hash_t>(_args)
  {
    parser = new jellyfish::parse_dna(files.begin(), files.end(),
                                      args->mer_len_arg, args->buffers_arg,
                                      args->buffer_size_arg);
    ary = new inv_hash_t::storage_t(args->size_arg, 2*args->mer_len_arg,
                                    args->counter_len_arg, 
                                    args->reprobes_arg, 
                                    jellyfish::quadratic_reprobes);
    if(args->matrix_given) {
      std::ifstream fd;
      fd.exceptions(std::ifstream::eofbit|std::ifstream::failbit|std::ifstream::badbit);
      fd.open(args->matrix_arg.c_str());
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
        dumper = new raw_inv_hash_dumper_t((uint_t)4, args->output_arg.c_str(),
                                           args->out_buffer_size_arg, ary);
      } else {
        inv_hash_dumper_t *_dumper =
          new inv_hash_dumper_t(args->threads_arg, args->output_arg.c_str(),
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
  mer_counting_qual_fasta_hash(const std::vector<const char *> &files,
                               count_args &_args) :
    mer_counting<jellyfish::parse_qual_dna, inv_hash_t>(_args)
  {
    parser = new jellyfish::parse_qual_dna(files,
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
      fd.open(args->matrix_arg.c_str());
      SquareBinaryMatrix m(&fd);
      fd.close();
      ary->set_matrix(m);
    }
    hash = new inv_hash_t(ary);

    if(args->no_write_flag) {
      dumper = new jellyfish::noop_dumper();
    } else {
      if(args->raw_flag) {
        dumper = new raw_inv_hash_dumper_t((uint_t)4, args->output_arg.c_str(),
                                           args->out_buffer_size_arg, ary);
      } else {
        inv_hash_dumper_t *_dumper =
          new inv_hash_dumper_t(args->threads_arg, args->output_arg.c_str(),
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
  mer_counting_fasta_direct(const std::vector<const char *> &files,
                            count_args &_args) :
    mer_counting<jellyfish::parse_dna, direct_index_t>(_args)
  {
    parser = new jellyfish::parse_dna(files.begin(), files.end(),
                                      args->mer_len_arg, args->buffers_arg,
                                      args->buffer_size_arg);
    ary = new direct_index_t::storage_t(2 * args->mer_len_arg);
    hash = new direct_index_t(ary);
    if(args->no_write_flag) {
      dumper = new jellyfish::noop_dumper();
    } else {
      if(args->raw_flag)
        std::cerr << "Switch --raw not (yet) supported with direct indexing. Ignoring." << std::endl;
      dumper = new direct_index_dumper_t(args->threads_arg, args->output_arg.c_str(),
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
  mer_counting_quake(std::vector<const char *>,
                     count_args &_args) :
    mer_counting<jellyfish::parse_quake, fastq_hash_t>(_args)
  {
    parser = new jellyfish::parse_quake(args->file_arg,
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
      dumper = new raw_fastq_dumper_t(args->threads_arg, args->output_arg.c_str(),
                                      args->out_buffer_size_arg,
                                      ary);
    }
    hash->set_dumper(dumper);
    parser->set_canonical(args->both_strands_flag);
  }
};

int count_main(int argc, char *argv[])
{
  count_args args(argc, argv);

  if(args.mer_len_arg < 2 || args.mer_len_arg > 31)
    die << "Invalid mer length '" << args.mer_len_arg
        << "'. It must be in [2, 31].";

  Time start;
  mer_counting_base *counter;
  if(!args.buffers_given)
    args.buffers_arg = 20 * args.threads_arg;

  if(args.quake_flag) {
    counter = new mer_counting_quake(args.file_arg, args);
  } else if(ceilLog2((unsigned long)args.size_arg) > 2 * (unsigned long)args.mer_len_arg) {
    counter = new mer_counting_fasta_direct(args.file_arg, args);
  } else if(args.min_quality_given) {
    counter = new mer_counting_qual_fasta_hash(args.file_arg, args);
  } else {
    counter = new mer_counting_fasta_hash(args.file_arg, args);
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
