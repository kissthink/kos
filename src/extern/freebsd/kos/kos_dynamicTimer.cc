#include <cstdint>
#include "kern/DynamicTimer.h"

// If successful, a nonzero value is returned. If 0 is returned, the callout
// function is currently executing or it has already finished executing
int kos_cancel_callout(void (*func)(ptr_t), void* arg, int time, int safe) {
  // Block until the dynamic timer is cancelled if safe = 1
  TimerEntry* entry = new TimerEntry { func, arg, (mword)time, false };
  if (safe) return DynamicTimer::cancelSync(entry) ? 1 : 0;
  return DynamicTimer::cancel(entry) ? 1 : 0;
}

int kos_reset_callout(void (*func)(ptr_t), void* arg, int time,
                      int newTime, void (*newFunc)(ptr_t), void* newArg) {
  mword time1 = time < 0 ? (mword)1 : (mword)time;
  mword time2 = newTime < 0 ? (mword)1 : (mword)newTime;
  TimerEntry* oldEntry = new TimerEntry { func, arg, time1, false };
  TimerEntry* newEntry = new TimerEntry { newFunc, newArg, time2, false };
  return DynamicTimer::resetEntry(oldEntry, newEntry) ? 1 : 0;
}
