/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#pragma once

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
  extern const size_t *quadratic_reprobes;

}
