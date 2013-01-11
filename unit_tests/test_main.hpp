#ifndef __TEST_MAIN_HPP__
#define __TEST_MAIN_HPP__

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

uint64_t random_bits(int length);
inline uint64_t random_bits() { return random_bits(64); }

struct file_unlink {
  std::string path;
  file_unlink(const char* s) : path(s) { }
  file_unlink(const std::string& s) : path(s) { }

  ~file_unlink() { unlink(path.c_str()); }
};

#endif /* __TEST_MAIN_HPP__ */

