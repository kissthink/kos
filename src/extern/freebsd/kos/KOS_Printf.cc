#include "kos/Printf.h"

// KOS
#include "kern/OutputSafe.h"
#include "util/basics.h"

#if 0
// http://www.codeproject.com/Articles/514443/Debug-Print-in-Variadic-Template-Style#printf
static void safe_printf(const char* fmt) {
  while (*fmt) {
    if (*fmt == '%') {
      if (*(fmt+1) == '%') ++fmt;
      else ABORT1("missing arguments\n");
    }
    StdOut.out(*fmt++);
  }
}

template<typename T, typename... Args>
static void safe_printf(const char* fmt, T value, Args... args) {
  while (*fmt) {
    if (*fmt == '%') {
      if (*(fmt+1) == '%') ++fmt;
      else {
        StdOut.out(value);
        safe_printf(fmt + 1, args...);
        return;
      }
    }
    StdOut.out(*fmt++);
  }
  ABORT1("extra arguments provided\n");
}
#endif

void KOS_PutChar(char ch) {
  StdOut.out(ch);
}
