#include "util/Debug.h"

int __errno = 0;

extern "C" char *getenv(const char *name) {
  return nullptr;
}

extern "C" void* sbrk(int incr) {
  DBG::outln(DBG::Libc, "sbrk: ", incr);
  return nullptr;
}
