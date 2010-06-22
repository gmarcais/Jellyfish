#include "misc.hpp"

void die(const char *msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);

  exit(1);
}
