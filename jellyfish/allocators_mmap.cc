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
#include <jellyfish/allocators_mmap.hpp>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

void *allocators::mmap::realloc(size_t new_size) {
  void *new_ptr = MAP_FAILED;
  if(ptr == MAP_FAILED) {
    new_ptr     = ::mmap(NULL, new_size, PROT_WRITE|PROT_READ, 
			 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  } 
  // mremap is Linux specific
  // TODO: We must do something if it is not supported
#ifdef MREMAP_MAYMOVE
  else {
    new_ptr = ::mremap(ptr, size, new_size, MREMAP_MAYMOVE);
  }
#endif
  if(new_ptr == MAP_FAILED)
    return NULL;
  size = new_size;
  ptr  = new_ptr;
  return ptr;
}

size_t allocators::mmap::round_to_page(size_t _size) {
  static const long                                   pg_size = sysconf(_SC_PAGESIZE);
  return (_size / pg_size + (_size % pg_size != 0)) * pg_size;
}

void allocators::mmap::fast_zero() {
  tinfo  info[nb_threads];
  size_t pgsize   = round_to_page(1);
  size_t nb_pages = size / pgsize + (size % pgsize != 0);

  for(int i = 0; i < nb_threads; i++) {
    info[i].start = (char *)ptr + pgsize * ((i * nb_pages) / nb_threads);
    info[i].end   = (char *)ptr + pgsize * (((i + 1) * nb_pages) / nb_threads);
        
    info[i].pgsize = pgsize;
    pthread_create(&info[i].thid, NULL, _fast_zero, &info[i]);
  }
  for(int i = 0; i < nb_threads; i++)
    pthread_join(info[i].thid, NULL);
}

void * allocators::mmap::_fast_zero(void *_info) {
  tinfo *info = (tinfo *)_info;
      
  for(char *cptr = info->start; cptr < info->end; cptr += info->pgsize)
    *cptr = 0;

  return NULL;
}
