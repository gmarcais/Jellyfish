#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>

#include <jellyfish/err.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/text_dumper.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jellyfish/stream_iterator.hpp>
#include <sub_commands/count_main_cmdline.hpp>

typedef jellyfish::cooperative::hash_counter<jellyfish::mer_dna> mer_array;
typedef jellyfish::text_dumper<mer_array::array> dumper;
typedef std::vector<const char*> file_vector;
typedef jellyfish::mer_overlap_sequence_parser<jellyfish::stream_iterator<file_vector::iterator> > sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, jellyfish::mer_dna> mer_iterator;

count_main_cmdline args; // Command line switches and arguments


template<typename PathIterator>
class mer_counter : public thread_exec {
  int                     nb_threads_;
  mer_array&              ary_;
  sequence_parser         parser_;

public:
  mer_counter(int nb_threads, mer_array& ary, PathIterator file_begin, PathIterator file_end) :
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
    fprintf(stderr, "id:%d count:%ld\n", thid, count);
  }
};

int count_main(int argc, char *argv[])
{
  args.parse(argc, argv);
  jellyfish::mer_dna::k(args.mer_len_arg);

  mer_array ary(args.size_arg, args.mer_len_arg, args.counter_len_arg, args.threads_arg, args.reprobes_arg);
  dumper dump(args.threads_arg, args.output_arg, ary.ary());
  mer_counter<file_vector::iterator> counter(args.threads_arg, ary, args.file_arg.begin(), args.file_arg.end());

  counter.exec_join(args.threads_arg);

  dump.dump();

  return 0;
}
