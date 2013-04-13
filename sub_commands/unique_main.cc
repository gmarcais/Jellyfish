#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jflib/multiplexed_io.hpp>
#include <sub_commands/unique_main_cmdline.hpp>

typedef jellyfish::mer_overlap_sequence_parser<std::ifstream*> sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, jellyfish::mer_dna> mer_iterator;

static unique_main_cmdline args;

int unique_main(int argc, char *argv[])
{
  args.parse(argc, argv);
  jellyfish::mer_dna::k(args.mer_arg);


  return 0;
}
