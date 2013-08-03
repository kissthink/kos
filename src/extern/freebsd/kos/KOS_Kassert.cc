#include "kos/Kassert.h"

// KOS
#include "kern/OutputSafe.h"
#include "mach/platform.h"

extern "C" void KOS_Reboot() {
  Reboot();
}

extern "C" void KOS_KassertPrint(const char* const loc, int line, const char* const func) {
  kassertprint(loc, line, func);
}

extern "C" void KOS_KassertPrint1(const char* const msg) {
  kassertprint1(msg);
}

extern "C" void KOS_KassertPrint2(const unsigned long long num) {
  kassertprint1(num);
}

// TODO import FmtHex version
