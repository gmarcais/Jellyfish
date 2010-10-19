#ifndef __JELLYFISH_STORAGE_HPP__
#define __JELLYFISH_STORAGE_HPP__

#include <stdlib.h>
#include <stdint.h>
#include <jellyfish/misc.hpp>

namespace jellyfish {

  class storage_t {
  public:
    storage_t() {}
    virtual ~storage_t() {}
  };

  // Entry 0 is used only when switching to a large field
  extern size_t *quadratic_reprobes;
  
}

#endif // __STORAGE_HPP__
