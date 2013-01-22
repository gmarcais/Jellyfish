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

#include <jellyfish/file_header.hpp>
#include <jellyfish/merge_files.hpp>

#include <sub_commands/merge_main_cmdline.hpp>


int merge_main(int argc, char *argv[])
{
  jellyfish::file_header out_header;
  out_header.fill_standard();
  out_header.set_cmdline(argc, argv);

  merge_main_cmdline args(argc, argv);

  try {
    merge_files(args.input_arg, args.output_arg, out_header);
  } catch(MergeError e) {
    die << e.what();
  }

  return 0;
}
