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

#ifndef __JELLYFISH_ALLOCATORS_SHM_HPP__
#define __JELLYFISH_ALLOCATORS_SHM_HPP__

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

namespace allocators {
  class shm
  {
    int         fd;
    void        *ptr;
    size_t      size;
    std::string name;
    bool        unlink;

  public:
    shm() : fd(-1), ptr(MAP_FAILED), size(0), name(""),
                      unlink(true) {}
    explicit shm(size_t _size) :
      fd(-1), ptr(MAP_FAILED), size(0), name(""), unlink(true) {
      realloc(_size);
    }
    shm(size_t _size, std::string _name) : 
      fd(-1), ptr(MAP_FAILED), size(0), name(_name), unlink(false) {
      realloc(_size);
    }
    ~shm() {
      if(ptr != MAP_FAILED)
        ::munmap(ptr, size);
      if(fd != -1) {
        close(fd);
        if(unlink)
          ::shm_unlink(name.c_str());
      }
    }

    void *get_ptr() { return ptr != MAP_FAILED ? ptr : NULL; }
    size_t get_size() const { return size; }

    void *realloc(size_t new_size) {
      size_t old_size = size;
      if(fd == -1) {
        // Create a temporary file name. Security considerations?
        if(name.empty()) {
          std::ostringstream name_stream("/shm_alloc_");
          name_stream << getpid() << "_" << (void *)this;
          name = name_stream.str();
        }
        fd = ::shm_open(name.c_str(), O_RDWR|O_CREAT|O_EXCL,
			S_IRUSR|S_IWUSR);
        if(fd == -1)
          return NULL;
      } else if(ptr != MAP_FAILED) {
        ::munmap(ptr, old_size);
        ptr = MAP_FAILED;
      }

      if(ftruncate(fd, new_size) == -1)
        return NULL;

      void *new_ptr = ::mmap(NULL, new_size, PROT_WRITE|PROT_READ,
			     MAP_SHARED, fd, 0);
      if(new_ptr == MAP_FAILED)
        return NULL;
      ptr = new_ptr;
      size = new_size;
      return ptr;
    }

    // Not yet implemented
    int lock() { return 0; }
    int unlock() { return 0; }
  };
};
#endif
