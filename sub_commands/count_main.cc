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

#include <cstdlib>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <memory>
#include <chrono>

#include <jellyfish/err.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/stream_manager.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jellyfish/jellyfish.hpp>
#include <jellyfish/merge_files.hpp>
#include <jellyfish/mer_dna_bloom_counter.hpp>
#include <jellyfish/generator_manager.hpp>
#include <sub_commands/count_main_cmdline.hpp>

using std::chrono::system_clock;
using std::chrono::duration;
using std::chrono::duration_cast;

template<typename DtnType>
inline double as_seconds(DtnType dtn) { return duration_cast<duration<double>>(dtn).count(); }

using jellyfish::mer_dna;
using jellyfish::mer_dna_bloom_counter;
typedef std::vector<const char*> file_vector;
typedef jellyfish::mer_overlap_sequence_parser<jellyfish::stream_manager<file_vector::const_iterator> > sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, mer_dna> mer_iterator;

static count_main_cmdline args; // Command line switches and arguments

// k-mer filters
struct filter_true {
  template<typename T>
  bool operator()(const T& x) const { return true; }
};

struct filter_bf {
  const mer_dna_bloom_counter& counter_;
  filter_bf(const mer_dna_bloom_counter& counter) : counter_(counter) { }
  bool operator()(const mer_dna& m) const {
    unsigned int c = counter_.check(m);
    return c > 1;
  }
};

enum OPERATION { COUNT, PRIME, UPDATE };
template<typename PathIterator, typename FilterType>
class mer_counter : public jellyfish::thread_exec {
  int                                     nb_threads_;
  mer_hash&                               ary_;
  jellyfish::stream_manager<PathIterator> streams_;
  sequence_parser                         parser_;
  FilterType                              filter_;
  OPERATION                               op_;

public:
  mer_counter(int nb_threads, mer_hash& ary,
              PathIterator file_begin, PathIterator file_end,
              PathIterator pipe_begin, PathIterator pipe_end,
              uint32_t concurent_files,
              OPERATION op, FilterType filter = FilterType()) :
    ary_(ary),
    streams_(file_begin, file_end, pipe_begin, pipe_end, concurent_files),
    parser_(mer_dna::k(), streams_.nb_streams(), 3 * nb_threads, 4096, streams_),
    filter_(filter),
    op_(op)
  { }

  virtual void start(int thid) {
    size_t count = 0;

    switch(op_) {
    case COUNT:
      for(mer_iterator mers(parser_, args.canonical_flag) ; mers; ++mers) {
        if(filter_(*mers))
          ary_.add(*mers, 1);
        ++count;
      }
      break;

    case PRIME:
      for(mer_iterator mers(parser_, args.canonical_flag) ; mers; ++mers) {
        ary_.set(*mers);
        ++count;
      }
      break;

    case UPDATE:
      mer_dna tmp;
      for(mer_iterator mers(parser_, args.canonical_flag) ; mers; ++mers) {
        ary_.update_add(*mers, 1, tmp);
        ++count;
      }
      break;
    }

    ary_.done();
  }

private:
  // int                          nb_threads_;
  // mer_hash&                    ary_;
  // stream_manager<PathIterator> streams_;
  // sequence_parser              parser_;
  // FilterType                   filter_;
  // OPERATION                    op_;
};

mer_dna_bloom_counter load_bloom_filter(const char* path) {
  std::ifstream in(path, std::ios::in|std::ios::binary);
  jellyfish::file_header header(in);
  if(!in.good())
    die << "Failed to parse bloom filter file '" << path << "'";
  if(header.format() != "bloomcounter")
    die << "Invalid format '" << header.format() << "'. Expected 'bloomcounter'";
  if(header.key_len() != mer_dna::k() * 2)
    die << "Invalid mer length in bloom filter";
  jellyfish::hash_pair<mer_dna> fns(header.matrix(1), header.matrix(2));
  mer_dna_bloom_counter res(header.size(), header.nb_hashes(), in, fns);
  if(!in.good())
    die << "Bloom filter file is truncated";
  in.close();
  return res;
}

// If get a termination signal, kill the manager and then kill myself.
static pid_t manager_pid = 0;
static void signal_handler(int sig) {
  if(manager_pid)
    kill(manager_pid, SIGTERM);
  signal(sig, SIG_DFL);
  kill(getpid(), sig);
  _exit(EXIT_FAILURE); // Should not be reached
}

int count_main(int argc, char *argv[])
{
  auto start_time = system_clock::now();

  jellyfish::file_header header;
  header.fill_standard();
  header.set_cmdline(argc, argv);

  args.parse(argc, argv);
  mer_dna::k(args.mer_len_arg);

  std::unique_ptr<jellyfish::generator_manager> generator_manager;
  if(args.generator_given) {
    auto gm =
      new jellyfish::generator_manager(args.generator_arg, args.Generators_arg,
                                       args.shell_given ? args.shell_arg : (const char*)0);
    generator_manager.reset(gm);
    generator_manager->start();
    manager_pid = generator_manager->pid();
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = signal_handler;
    assert(sigaction(SIGTERM, &act, 0) == 0);
  }

  mer_hash ary(args.size_arg, args.mer_len_arg * 2, args.counter_len_arg, args.threads_arg, args.reprobes_arg);
  if(args.disk_flag)
    ary.do_size_doubling(false);

  std::auto_ptr<jellyfish::dumper_t<mer_array> > dumper;
  if(args.text_flag)
    dumper.reset(new text_dumper(args.threads_arg, args.output_arg, &header));
  else
    dumper.reset(new binary_dumper(args.out_counter_len_arg, ary.key_len(), args.threads_arg, args.output_arg, &header));
  ary.dumper(dumper.get());

  auto after_init_time = system_clock::now();

  OPERATION do_op = COUNT;
  if(args.if_given) {
    mer_counter<file_vector::const_iterator, filter_true> counter(args.threads_arg, ary,
                                                                  args.if_arg.begin(), args.if_arg.end(),
                                                                  args.if_arg.end(), args.if_arg.end(), // no multi pipes
                                                                  args.Files_arg, PRIME);
    counter.exec_join(args.threads_arg);
    do_op = UPDATE;
  }

  // Iterators to the multi pipe paths. If no generator manager,
  // generate an empty range.
  auto pipes_begin = generator_manager.get() ? generator_manager->pipes().begin() : args.file_arg.end();
  auto pipes_end = (bool)generator_manager ? generator_manager->pipes().end() : args.file_arg.end();
  if(args.bf_given) {
    mer_dna_bloom_counter bc = load_bloom_filter(args.bf_arg);
    mer_counter<file_vector::const_iterator, filter_bf>  counter(args.threads_arg, ary,
                                                                 args.file_arg.begin(), args.file_arg.end(),
                                                                 pipes_begin, pipes_end,
                                                                 args.Files_arg,
                                                                 do_op, filter_bf(bc));
    counter.exec_join(args.threads_arg);
  } else {
    mer_counter<file_vector::const_iterator, filter_true> counter(args.threads_arg, ary,
                                                                  args.file_arg.begin(), args.file_arg.end(),
                                                                  pipes_begin, pipes_end,
                                                                  args.Files_arg, do_op);
    counter.exec_join(args.threads_arg);
  }

  // If we have a manager, wait for it
  if(generator_manager) {
    signal(SIGTERM, SIG_DFL);
    manager_pid = 0;
    if(!generator_manager->wait())
      die << "Some generator commands failed";
    generator_manager.reset();
  }

  auto after_count_time = system_clock::now();

  // If no intermediate files, dump directly into output file. If not, will do a round of merging
  if(!args.no_write_flag) {
    if(dumper->nb_files() == 0) {
      dumper->one_file(true);
      if(args.lower_count_given)
        dumper->min(args.lower_count_arg);
      if(args.upper_count_given)
        dumper->max(args.upper_count_arg);
      dumper->dump(ary.ary());
    } else {
      dumper->dump(ary.ary());
      if(!args.no_merge_flag) {
        std::vector<const char*> files = dumper->file_names_cstr();
        uint64_t min = args.lower_count_given ? args.lower_count_arg : 0;
        uint64_t max = args.upper_count_given ? args.upper_count_arg : std::numeric_limits<uint64_t>::max();
        try {
          merge_files(files, args.output_arg, header, min, max);
        } catch(MergeError e) {
          die << e.what();
        }
        if(!args.no_unlink_flag) {
          for(int i =0; i < dumper->nb_files(); ++i)
            unlink(files[i]);
        }
      } // if(!args.no_merge_flag
    } // if(!args.no_merge_flag
  }

  auto after_dump_time = system_clock::now();

  if(args.timing_given) {
    std::ofstream timing_file(args.timing_arg);
    timing_file << "Init     " << as_seconds(after_init_time - start_time) << "\n"
                << "Counting " << as_seconds(after_count_time - after_init_time) << "\n"
                << "Writing  " << as_seconds(after_dump_time - after_count_time) << "\n";
  }

  return 0;
}
