/* Allocators return zeroed chunk of memory
 */
#ifndef __ALLOCATORS_MALLOC_HPP__
#define __ALLOCATORS_MALLOC_HPP__

#include <stdlib.h>

namespace allocators
{
   class malloc
  {
    void        *ptr;
    size_t      size;

  public:
    malloc() : ptr(NULL), size(0) {}
    malloc(size_t _size) : ptr(NULL), size(0) {
        realloc(_size);
    }
    ~malloc() {
      if(ptr)
        ::free(ptr);
    }
    
    void *get_ptr() { return ptr; }
    size_t get_size() { return size; }

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
  };
}
#endif
