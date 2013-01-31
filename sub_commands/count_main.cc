#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <memory>

#include <jellyfish/err.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jellyfish/stream_iterator.hpp>
#include <jellyfish/jellyfish.hpp>
#include <jellyfish/merge_files.hpp>
#include <sub_commands/count_main_cmdline.hpp>

typedef std::vector<const char*> file_vector;
typedef jellyfish::mer_overlap_sequence_parser<jellyfish::stream_iterator<file_vector::iterator> > sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, jellyfish::mer_dna> mer_iterator;

static count_main_cmdline args; // Command line switches and arguments


template<typename PathIterator>
class mer_counter : public thread_exec {
  int                     nb_threads_;
  mer_hash&               ary_;
  sequence_parser         parser_;

public:
  mer_counter(int nb_threads, mer_hash& ary, PathIterator file_begin, PathIterator file_end) :
    ary_(ary),
    parser_(jellyfish::mer_dna::k(), 3 * nb_threads, 4096, jellyfish::stream_iterator<PathIterator>(file_begin, file_end),
            jellyfish::stream_iterator<PathIterator>())
  { }

  virtual void start(int thid) {
    size_t count = 0;
    for(mer_iterator mers(parser_, args.canonical_flag) ; mers; ++mers) {
      ary_.add(*mers, 1);
      ++count;
    }
    ary_.done();
  }
};

int count_main(int argc, char *argv[])
{
  jellyfish::file_header header;
  header.fill_standard();
  header.set_cmdline(argc, argv);

  args.parse(argc, argv);
  jellyfish::mer_dna::k(args.mer_len_arg);

  mer_hash ary(args.size_arg, args.mer_len_arg * 2, args.counter_len_arg, args.threads_arg, args.reprobes_arg);
  if(args.disk_flag)
    ary.do_size_doubling(false);

  std::auto_ptr<jellyfish::dumper_t<mer_array> > dumper;
  if(args.text_flag)
    dumper.reset(new text_dumper(args.threads_arg, args.output_arg, &header));
  else
    dumper.reset(new binary_dumper(args.out_counter_len_arg, ary.key_len(), args.threads_arg, args.output_arg, &header));
  ary.dumper(dumper.get());

  mer_counter<file_vector::iterator> counter(args.threads_arg, ary, args.file_arg.begin(), args.file_arg.end());
  counter.exec_join(args.threads_arg);

  // If no intermediate files, dump directly into output file. If not, will do a round of merging
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
  } // else {

  return 0;
}
