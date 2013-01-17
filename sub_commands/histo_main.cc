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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/fstream_default.hpp>
#include <jellyfish/jellyfish.hpp>
#include <sub_commands/histo_main_cmdline.hpp>


int histo_main(int argc, char *argv[])
{
  histo_main_cmdline args(argc, argv);

  std::ifstream is(args.db_arg);
  if(!is.good())
    die << "Failed to open input file '" << args.db_arg << "'" << err::no;
  jellyfish::file_header header;
  header.read(is);
  if(header.format().compare(binary_dumper::format))
    die << "Unknown format '" << header.format() << "'";
  jellyfish::mer_dna::k(header.key_len() / 2);
  binary_reader reader(is, &header);

  if(args.low_arg < 1)
    args.error("Low count value must be >= 1");
  if(args.high_arg < args.low_arg)
    args.error("High count value must be >= to low count value");
  ofstream_default out(args.output_given ? args.output_arg : 0, std::cout);
  if(!out.good())
    die << "Error opening output file '" << args.output_arg << "'" << err::no;

  const uint64_t base =
    args.low_arg > 1 ? (args.increment_arg >= args.low_arg ? 1 : args.low_arg - args.increment_arg) : 1;
  const uint64_t ceil = args.high_arg + args.increment_arg;
  const uint64_t inc = args.increment_arg;

  const uint64_t  nb_buckets = (ceil + inc - base) / inc;
  uint64_t* histo      = new uint64_t[nb_buckets];
  memset(histo, '\0', sizeof(uint64_t) * nb_buckets);

  while(reader.next()) {
    if(reader.val() < base)
      ++histo[0];
    else if(reader.val() > ceil)
      ++histo[nb_buckets - 1];
    else
      ++histo[(reader.val() - base) / inc];
  }

  for(uint64_t i = 0, col = base; i < nb_buckets; ++i, col += inc)
    if(histo[i] > 0 || args.full_flag)
      out << col << " " << histo[i] << "\n";

  delete [] histo;
  out.close();

  return 0;
}
