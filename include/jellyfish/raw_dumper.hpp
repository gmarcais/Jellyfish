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

#ifndef __JELLYFISH_RAW_DUMPER_HPP__
#define __JELLYFISH_RAW_DUMPER_HPP__

#include <limits>

#include <jellyfish/file_header.hpp>
#include <jellyfish/dumper.hpp>

namespace jellyfish {
template<typename storage_t>
class raw_dumper : public dumper_t<storage_t> {
  typedef dumper_t<storage_t> super;
  const char*  prefix_;
  file_header* header_;

public:
  static const char* format;
  raw_dumper(const char* prefix, file_header* header = 0) :
    prefix_(prefix),
    header_(header)
  { }

  virtual void _dump(storage_t* ary) {
    std::ofstream out;
    super::open_next_file(prefix_, out);

    if(header_) {
      header_->update_from_ary(*ary);
      header_->format(format);
      header_->intermediary(super::nb_files());
      header_->write(out);
    }

    char* data;
    size_t size;
    ary->block_to_ptr(0, std::numeric_limits<size_t>::max(),
                      &data, &size);
    out.write(data, size);
    ary->clear();
  }
};
template<typename storage_t>
const char* raw_dumper<storage_t>::format = "binary/raw";

}

#endif /* __JELLYFISH_RAW_DUMPER_HPP__ */
