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
#include <jellyfish/misc.hpp>

uint64_t bogus_sum(void *data, size_t len) {
  uint64_t res = 0, tmp = 0;
  uint64_t *ptr = (uint64_t *)data;

  while(len >= sizeof(uint64_t)) {
    res ^= *ptr++;
    len -= sizeof(uint64_t);
  }
  
  if(len > 0) {
    memcpy(&tmp, ptr, len);
    res ^= tmp;
  }
  return res;
}
