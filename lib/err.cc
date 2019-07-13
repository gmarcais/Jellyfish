/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#include <jellyfish/err.hpp>

namespace jellyfish {
namespace err {
  std::ostream &operator<<(std::ostream &os, const substr &ss) {
    os.write(ss._s, ss._l);
    return os;
  }

  std::ostream &operator<<(std::ostream &os, const no_t &x) {
    x.write(os, errno);
    return os;
  }
}
}
