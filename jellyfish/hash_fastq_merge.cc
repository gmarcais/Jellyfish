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

#include <jellyfish/hash_fastq_merge_cmdline.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/mer_counting.hpp>
#include <jellyfish/fastq_dumper.hpp>

int hash_fastq_merge_main(int argc, char *argv[])
{
  hash_fastq_merge_args args(argc, argv);

  fastq_hash_t::storage_t ary(args.size_arg, 2*args.mer_len_arg,
                              args.reprobes_arg, jellyfish::quadratic_reprobes);
  fastq_hash_t hash(&ary);
  fastq_hash_t::thread_ptr_t counter(hash.new_thread());
  
  int num_hashes = args.db_arg.size();
  for(int i = 0; i < num_hashes; ++i) {
    fastq_storage_t *ihash = raw_fastq_dumper_t::read(args.db_arg[i]);
    if(ihash->get_key_len() != hash.get_key_len())
      die << "Invalid mer length '" << (ihash->get_key_len() / 2)
          << "' for database '" << args.db_arg[i] 
          << "'. Should be '" << (hash.get_key_len() / 2) << "'";
    fastq_storage_t::iterator iit = ihash->iterator_all();
    while(iit.next()) {
      counter->add(iit.get_key(), iit.get_val().to_float());
    }
    delete ihash;
  }

  raw_fastq_dumper_t dumper(4, args.output_arg, 
                            args.out_buffer_size_arg, &ary);
  hash.set_dumper(&dumper);
  hash.dump();

  return 0;
}
