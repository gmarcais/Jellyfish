#ifndef __TEST_MAIN_HPP__
#define __TEST_MAIN_HPP__

#include <stdint.h>
#include <stdlib.h>

uint64_t random_bits(int length);
inline uint64_t random_bits() { return random_bits(64); }

#endif /* __TEST_MAIN_HPP__ */

