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

#ifndef __JELLYFISH_ALLOCATORS_MMAP_HPP__
#define __JELLYFISH_ALLOCATORS_MMAP_HPP__

#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

namespace allocators {
  class mmap {
    void   *ptr;
    size_t  size;
   
  public:
    mmap() : ptr(MAP_FAILED), size(0) {}
    explicit mmap(size_t _size) : ptr(MAP_FAILED), size(0) {
      realloc(_size);
      fast_zero();
    }
    ~mmap() {
      if(ptr != MAP_FAILED)
        ::munmap(ptr, size);
    }

    void *get_ptr() const { return ptr != MAP_FAILED ? ptr : NULL; }
    size_t get_size() const { return size; }
    void *realloc(size_t new_size);
    int lock() { return mlock(ptr, size); }
    int unlock() { return munlock(ptr, size); }
    
    // Return a a number of bytes which is a number of whole pages at
    // least as large as size.
    static size_t round_to_page(size_t _size);

  private:
    static const int nb_threads = 4;
    struct tinfo {
      pthread_t  thid;
      char      *start, *end;
      size_t     pgsize;
    };
    void fast_zero();
    static void * _fast_zero(void *_info);
  };
}

#endif
