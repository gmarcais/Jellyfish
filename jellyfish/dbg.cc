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

#include <jellyfish/dbg.hpp>

namespace dbg {
#ifdef DEBUG

  pthread_mutex_t print_t::_lock = PTHREAD_MUTEX_INITIALIZER;
  std::ostream &operator<<(std::ostream &os, const dbg::substr &ss) {
    os.write(ss._s, ss._l);
    return os;
  }

#else

  std::ostream &operator<<(std::ostream &os, const dbg::substr &ss) {
    return os;
  }

#endif
}
