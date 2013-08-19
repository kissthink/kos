#include "kos/Delay.h"

// KOS
#include "mach/Delay.h"

void KOS_udelay(int usec) {
  Delay::udelay(usec);
}
