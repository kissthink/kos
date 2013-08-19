#include "sys/systm.h"
#include "kos/Delay.h"

void DELAY(int usec) {
  KOS_udelay(usec);
}
