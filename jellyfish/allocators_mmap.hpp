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

namespace allocators {
  class mmap
  {
    void        *ptr;
    size_t      size;
   
  public:
    mmap() : ptr(MAP_FAILED), size(0) {}
    mmap(size_t _size) : ptr(MAP_FAILED), size(0) {
      realloc(_size);
      fast_zero();
    }
    ~mmap() {
      if(ptr != MAP_FAILED)
        ::munmap(ptr, size);
    }

    void *get_ptr() const { return ptr != MAP_FAILED ? ptr : NULL; }
    size_t get_size() const { return size; }

    void *realloc(size_t new_size) {
      void *new_ptr = MAP_FAILED;
      if(ptr == MAP_FAILED) {
        new_ptr = ::mmap(NULL, new_size, PROT_WRITE|PROT_READ, 
			 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      } 
// mremap is Linux specific
#ifdef MREMAP_MAYMOVE
      else {
        new_ptr = ::mremap(ptr, size, new_size, MREMAP_MAYMOVE);
      }
#endif
      if(new_ptr == MAP_FAILED)
        return NULL;
      size = new_size;
      ptr = new_ptr;
      return ptr;
    }
    
  private:
    static const int nb_threads = 4;
    struct tinfo {
      pthread_t  thid;
      char      *start, *end;
      size_t     pgsize;
    };
    void fast_zero() {
      tinfo info[nb_threads];
      size_t pgsize = (size_t)getpagesize();
      size_t nb_pages = size / pgsize + (size % pgsize != 0);

      for(int i = 0; i < nb_threads; i++) {
        info[i].start  = (char *)ptr + pgsize * ((i * nb_pages) / nb_threads);
        info[i].end    = (char *)ptr + pgsize * (((i + 1) * nb_pages) / nb_threads);
        
        info[i].pgsize = pgsize;
        pthread_create(&info[i].thid, NULL, _fast_zero, &info[i]);
      }
      for(int i = 0; i < nb_threads; i++)
        pthread_join(info[i].thid, NULL);
    }

    static void * _fast_zero(void *_info) {
      tinfo *info = (tinfo *)_info;
      
      for(char *cptr = info->start; cptr < info->end; cptr += info->pgsize)
        *cptr = 0;

      return NULL;
    }
  };
};
#endif
