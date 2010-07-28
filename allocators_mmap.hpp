#ifndef __ALLOCATORS_MMAP_HPP__
#define __ALLOCATORS_MMAP_HPP__

#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>

namespace allocators {
  class mmap
  {
    void        *ptr;
    size_t      size;
   
  public:
    mmap() : ptr(MAP_FAILED), size(0) {}
    mmap(size_t _size) : ptr(MAP_FAILED), size(0) {
      realloc(_size);
    }
    ~mmap() {
      if(ptr != MAP_FAILED)
        ::munmap(ptr, size);
    }

    void *get_ptr() const { return ptr != MAP_FAILED ? ptr : NULL; }
    size_t get_size() const { return size; }

    void *realloc(size_t new_size) {
      size_t old_size = size;
      void *new_ptr;
      if(ptr == MAP_FAILED) {
        new_ptr = ::mmap(NULL, new_size, PROT_WRITE|PROT_READ, 
			 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      } else {
        new_ptr = ::mremap(ptr, old_size, new_size, MREMAP_MAYMOVE);
      }
      if(new_ptr == MAP_FAILED)
        return NULL;
      size = new_size;
      ptr = new_ptr;
      return ptr;
    }
  };
};
#endif
