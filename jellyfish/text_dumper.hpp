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

#ifndef __JELLYFISH_TEXT_DUMPER_HPP__
#define __JELLYFISH_TEXT_DUMPER_HPP__

#include <jellyfish/sorted_dumper.hpp>

namespace jellyfish {
template<typename storage_t>
class text_dumper : public sorted_dumper<text_dumper<storage_t>, storage_t> {
  typedef sorted_dumper<text_dumper<storage_t>, storage_t> super;

public:
  text_dumper(int nb_threads, const char* file_prefix, file_header* header = 0) :
    super(nb_threads, file_prefix, header)
  { }

  virtual void _dump(storage_t* ary) {
    if(super::header_) {
      super::header_->update_from_ary(*ary);
      super::header_->format("text/sorted");
    }
    super::_dump(ary);
  }

  void write_key_value_pair(std::ostream& out, typename super::heap_item item) {
    out << item->key_ << " " << item->val_ << "\n";
  }
};
}

#endif /* __JELLYFISH_TEXT_DUMPER_HPP__ */
