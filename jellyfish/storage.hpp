#ifndef __STORAGE_HPP__
#define __STORAGE_HPP__

#include <stdlib.h>
#include <stdint.h>
#include "misc.hpp"

namespace jellyfish {

  // Entry 0 is used only when switching to a large field
  extern size_t *quadratic_reprobes;
  
}

#endif // __STORAGE_HPP__
