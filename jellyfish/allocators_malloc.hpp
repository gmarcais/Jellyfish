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

/* Allocators return zeroed chunk of memory
 */
#ifndef __JELLYFISH_ALLOCATORS_MALLOC_HPP__
#define __JELLYFISH_ALLOCATORS_MALLOC_HPP__

#include <stdlib.h>

namespace allocators
{
   class malloc
  {
    void        *ptr;
    size_t      size;

  public:
    malloc() : ptr(NULL), size(0) {}
    explicit malloc(size_t _size) : ptr(NULL), size(0) {
        realloc(_size);
    }
    ~malloc() {
      if(ptr)
        ::free(ptr);
    }
    
    void *get_ptr() { return ptr; }
    size_t get_size() const { return size; }

    void *realloc(size_t new_size) {
      size_t old_size = size;
      void *new_ptr = ::realloc(ptr, new_size);
      if(!new_ptr)
        return NULL;
      ptr = new_ptr;
      size = new_size;
      if(new_size > old_size)
        ::memset((char *)ptr + old_size, '\0', new_size - old_size);
      return ptr;
    }

    // NOOP
    int lock() { return 0; }
    int unlock() { return 0; }
  };
}
#endif
