#include "kos/Timer.h"

// KOS
#include "mach/Machine.h"

int KOS_Ticks() {
  return Machine::now();  // PIT tick
}
