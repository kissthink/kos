#include "kos/kos_kassert.h"

// KOS
#include "kern/OutputSafe.h"
#include "mach/platform.h"

void kos_reboot() {
  Reboot();
}

void kos_kassertprint(const char* const loc, int line, const char* const func) {
  kassertprint(loc, line, func);
}

void kos_kassertprint1(const char* const msg) {
  kassertprint1(msg);
}

void kos_kassertprint2(const unsigned long long num) {
  kassertprint1(num);
}

// TODO import FmtHex version
