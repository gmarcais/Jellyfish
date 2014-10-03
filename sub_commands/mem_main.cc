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

#include <iostream>
#include <string>

#include <jellyfish/mer_dna.hpp>
#include <jellyfish/large_hash_array.hpp>
#include <sub_commands/mem_main_cmdline.hpp>

static const char* suffixes = "kMGTPE";

template<uint64_t U>
std::string add_suffix(uint64_t x) {
  const int max_i = strlen(suffixes);
  int       i     = 0;
  while(x >= U && i <= max_i) {
    x /= U;
    ++i;
  }
  std::ostringstream res;
  res << x;
  if(i > 0)
    res << suffixes[i - 1];
  return res.str();
}

int mem_main(int argc, char *argv[]) {
  mem_main_cmdline args(argc, argv);
  jellyfish::large_hash::array<jellyfish::mer_dna>::usage_info usage(args.mer_len_arg * 2, args.counter_len_arg, args.reprobes_arg);

  if(args.size_given) {
    uint64_t val = usage.mem(args.size_arg);
    std::cout << val << " (" << add_suffix<1024>(val) << ")\n";
  } else {
    uint64_t val = usage.size(args.mem_arg);
    std::cout << val << " (" << add_suffix<1000>(val) << ")\n";
  }

  return 0;
}
