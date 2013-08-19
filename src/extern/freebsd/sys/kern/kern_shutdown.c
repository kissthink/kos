#include "sys/systm.h"
#include "machine/stdarg.h"
#include "kos/Kassert.h"

const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors. It prints "panic: mseg",
 * and then reboots. If we are called twice, then we avoid trying to sync
 * the disks as this often leads to recursive panics.
 */
void
panic(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf(fmt, ap);
  va_end(ap);

  KOS_Reboot();
  for (;;) {}   // suppress warning
}
