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
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/compacted_hash.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/counter.hpp>
#include <jellyfish/histo_main_cmdline.hpp>
#include <jellyfish/fstream_default.hpp>

template<typename hash_t>
class histogram : public thread_exec {
  const hash_t   *hash;
  const uint_t    threads;
  const uint64_t  base, ceil, inc, nb_buckets, nb_slices;
  uint64_t       *data;
  counter_t       slice_id;

public:
  histogram(const hash_t *_hash, uint_t _threads, uint64_t _base,
            uint64_t _ceil, uint64_t _inc) :
    hash(_hash), threads(_threads), base(_base), ceil(_ceil), inc(_inc), 
    nb_buckets((ceil + inc - base) / inc), nb_slices(threads * 100)
  {
    data = new uint64_t[threads * nb_buckets];
    memset(data, '\0', threads * nb_buckets * sizeof(uint64_t));
  }

  ~histogram() {
    if(data)
      delete [] data;
  }

  void do_it() {
    exec_join(threads);
  }

  void start(int th_id) {
    uint64_t *hist = &data[th_id * nb_buckets];

    for(size_t i = slice_id++; i <= nb_slices; i = slice_id++) {
      typename hash_t::iterator it = hash->iterator_slice(i, nb_slices);
      while(it.next()) {
        if(it.get_val() < base)
          hist[0]++;
        else if(it.get_val() > ceil)
          hist[nb_buckets - 1]++;
        else
          hist[(it.get_val() - base) / inc]++;
      }
    }
  }

  void print(std::ostream &out, bool full) {
    uint64_t col = base;
    for(uint64_t i = 0; i < nb_buckets; i++, col += inc) {
      uint64_t count = 0;
      for(uint_t j = 0; j < threads; j++)
        count += data[j * nb_buckets + i];
      if(count > 0 || full)
        out << col << " " << count << "\n";
    }
  }
};

int histo_main(int argc, char *argv[])
{
  histo_args args(argc, argv);

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

  mapped_file dbf(args.db_arg.c_str());
  dbf.sequential().will_need();
  char type[8];
  memcpy(type, dbf.base(), sizeof(type));
  if(!strncmp(type, jellyfish::raw_hash::file_type, sizeof(type))) {
    raw_inv_hash_query_t hash(dbf);
    histogram<raw_inv_hash_query_t> histo(&hash, args.threads_arg, base, ceil, args.increment_arg);
    histo.do_it();
    histo.print(out, args.full_flag);
  } else if(!strncmp(type, jellyfish::compacted_hash::file_type, sizeof(type))) {
    hash_query_t hash(dbf);
    if(args.verbose_flag)
      std::cerr << "mer length  = " << hash.get_mer_len() << "\n"
                << "hash size   = " << hash.get_size() << "\n"
                << "max reprobe = " << hash.get_max_reprobe() << "\n"
                << "matrix      = " << hash.get_hash_matrix().xor_sum() << "\n"
                << "inv_matrix  = " << hash.get_hash_inverse_matrix().xor_sum() << "\n";
    histogram<hash_query_t> histo(&hash, args.threads_arg, base, ceil, args.increment_arg);
    histo.do_it();
    histo.print(out, args.full_flag);
  } else {
    die << "Invalid file type '" << err::substr(type, sizeof(type)) << "'.";
  }
  out.close();

  return 0;
}
