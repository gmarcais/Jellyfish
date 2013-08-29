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
#include <fstream>
#include <stdexcept>
#include <memory>
#include <vector>

#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/err.hpp>

namespace jellyfish {
template<typename PathIterator>
class stream_manager {
  typedef std::unique_ptr<std::istream> stream_type;

  PathIterator paths_cur_, paths_end_;
  locks::pthread::mutex     mutex_;

public:
  define_error_class(Error);

  stream_manager(PathIterator paths_begin, PathIterator paths_end) :
    paths_cur_(paths_begin), paths_end_(paths_end)
  { }

  stream_type next() {
    locks::pthread::mutex_lock lock(mutex_);
    stream_type res;
    if(paths_cur_ != paths_end_) {
      res.reset(new std::ifstream(*paths_cur_));
      ++paths_cur_;
      if(!res)
        eraise(Error) << "Can't open file '" << *paths_cur_ << "'";
    }

    return res;
  }
};
} // namespace jellyfish
