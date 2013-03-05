#include "util/Debug.h"
#include "kern/KernelHeap.h"

int __errno = 0;

static vaddr memstart = 0;
static vaddr memend = 0;
static vaddr memptr = 0;
static size_t memsize = (1<<16);

extern "C" char *getenv(const char *name) {
  DBG::outln(DBG::Libc, "LIBC/getenv: ", name);
  return nullptr;
}

extern "C" void* sbrk(int incr) {
  if (memstart == 0) {
    memstart = KernelHeap::alloc(memsize);
    memend = memstart + memsize;
    memptr = memstart;
  }
  memptr += incr;
  DBG::outln(DBG::Libc, "LIBC/sbrk: ", ptr_t(memptr));
  KASSERT(memptr <= memend, memptr);
  return ptr_t(memptr - incr);
//  memptr -= incr;
//  errno = ENOMEM;
//  return ptr_t(-1);
}
