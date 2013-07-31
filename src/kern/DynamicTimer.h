#ifndef _DynamicTimer_h_
#define _DynamicTimer_h_

#include "extern/stl/mod_set"
#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"
#include "mach/Machine.h"

#undef __STRICT_ANSI__  // to get access to vsnprintf
#include <cstdio>

struct TimerEntry {
  void (*func)(ptr_t);
  void* arg;
  mword time;
};

struct DynamicTimerCompare {
  inline bool operator()(const TimerEntry* e1, const TimerEntry* e2) {
    return e1->time < e2->time || e1 < e2;
  }
};

class DynamicTimer {
  static SpinLock lk;
  using TimerSet = set<TimerEntry*,DynamicTimerCompare,KernelAllocator<TimerEntry*>>;
  static TimerSet entries;
  static Thread* thread;

  DynamicTimer() = delete;
  DynamicTimer(const DynamicTimer&) = delete;
  DynamicTimer& operator=(const DynamicTimer&) = delete;
public:
  static void init();
  static void addEvent(TimerEntry* entry) {
    ScopedLock<> lo(lk);
    entries.insert(entry);
    kernelScheduler.start(*thread);
  }
  static void run();
  static Thread* getThread() {
    return thread;
  }
  static inline bool empty() {
    ScopedLock<> lo(lk);
    return entries.empty();
  }
};

#endif /* _DynamicTimer_h_ */
