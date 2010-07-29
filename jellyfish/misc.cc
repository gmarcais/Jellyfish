#include <config.h>
#include "misc.hpp"

void die(const char *msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);

  exit(1);
}

uint64_t bogus_sum(void *data, size_t len) {
  uint64_t res = 0, tmp = 0;
  uint64_t *ptr = (uint64_t *)data;

  while(len >= sizeof(uint64_t)) {
    res ^= *ptr++;
    len -= sizeof(uint64_t);
  }
  
  if(len > 0) {
    memcpy(&tmp, ptr, len);
    res ^= tmp;
  }
  return res;
}
