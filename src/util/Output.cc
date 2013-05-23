#include "util/Output.h"
#include "kern/globals.h"

#include <iomanip>

#undef __STRICT_ANSI__ // to get access to vsnprintf
#include <cstdio>

std::ostream& operator<<(std::ostream &os, const FmtHex& hex) {
  os << std::setw(hex.digits) << hex.ptr;
  return os;
}

std::ostream& operator<<(std::ostream &os, const Printf& pf) {
  char* buffer = nullptr;
  int size = vsnprintf(buffer, 0, pf.format, pf.args);
  if (size >- 1) {
    size += 1; 
    buffer = new char[size];
    vsnprintf(buffer, size, pf.format, pf.args);
    os << buffer;
    globaldelete(buffer, size);
  }
  return os;
}
