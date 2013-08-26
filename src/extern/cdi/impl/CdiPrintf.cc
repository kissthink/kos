#include "cdi/printf.h"

// KOS
#include "kern/Debug.h"
#include "util/Output.h"
#include <cstdarg>

#undef __STRICT_ANSI__  // to get access to vsnprintf
#include <cstdio>

extern void globaldelete(ptr_t addr, size_t size);

void CdiPrintf(const char* Format, ...) {
  va_list args, tmpargs;
  va_start(args, Format);
  va_copy(tmpargs, args);
  int size = vsnprintf(nullptr, 0, Format, tmpargs);
  va_end(tmpargs);
  if (size < 0) return;
  size += 1;
  char* buffer = new char[size];
  vsnprintf(buffer, size, Format, args);
  DBG::out(DBG::CDI, buffer);
  globaldelete(buffer, size);
  va_end(args);
}
