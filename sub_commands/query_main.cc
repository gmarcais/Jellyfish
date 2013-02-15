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

#include <vector>

#include <jellyfish/err.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/file_header.hpp>
#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jellyfish/stream_iterator.hpp>
#include <jellyfish/mer_dna_bloom_counter.hpp>
#include <sub_commands/query_main_cmdline.hpp>

using jellyfish::mer_dna;
using jellyfish::mer_dna_bloom_counter;
typedef std::vector<const char*> file_vector;
typedef jellyfish::mer_overlap_sequence_parser<jellyfish::stream_iterator<file_vector::iterator> > sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, mer_dna> mer_iterator;

static query_main_cmdline args;

mer_dna_bloom_counter query_load_bloom_filter(const char* path) {
  std::ifstream in(path, std::ios::in|std::ios::binary);
  jellyfish::file_header header(in);
  if(!in.good())
    die << "Failed to parse bloom filter file '" << path << "'";
  if(header.format() != "bloomcounter")
    die << "Invalid format '" << header.format() << "'. Expected 'bloomcounter'";
  mer_dna::k(header.key_len() / 2);
  jellyfish::hash_pair<mer_dna> fns(header.matrix(1), header.matrix(2));
  mer_dna_bloom_counter res(header.size(), header.nb_hashes(), in, fns);
  if(!in.good())
    die << "Bloom filter file is truncated";
  in.close();
  return res;
}

template<typename PathIterator>
void query_from_sequence(PathIterator file_begin, PathIterator file_end, const mer_dna_bloom_counter& filter) {
  sequence_parser parser(mer_dna::k(), 3, 4096, jellyfish::stream_iterator<PathIterator>(file_begin, file_end),
                         jellyfish::stream_iterator<PathIterator>());
  for(mer_iterator mers(parser, args.canonical_flag) ; mers; ++mers)
    std::cout << *mers << " " << filter.check(*mers) << "\n";
}

int query_main(int argc, char *argv[])
{
  args.parse(argc, argv);

  mer_dna_bloom_counter filter = query_load_bloom_filter(args.file_arg);

  query_from_sequence(args.sequence_arg.begin(), args.sequence_arg.end(), filter);

  mer_dna m;
  for(auto it = args.mers_arg.cbegin(); it != args.mers_arg.cend(); ++it) {
    try {
      m = *it;
      std::cout << m << " " << filter.check(m);
    } catch(std::length_error e) {
      std::cerr << "Invalid mer " << m << "\n";
    }
  }

  return 0;
}
