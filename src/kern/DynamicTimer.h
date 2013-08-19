#ifndef _TimerHandler_h_
#define _TimerHandler_h_ 1

#include "kern/Kernel.h"
#include "kern/TimerEvent.h"
#include "ipc/SpinLock.h"
#include <list>

class DynamicTimer {
  DynamicTimer() = delete;
  DynamicTimer(const DynamicTimer&) = delete;
  DynamicTimer& operator=(const DynamicTimer&) = delete;

  static SpinLock lk;
  static std::list<TimerEvent*,KernelAllocator<TimerEvent*>> events;
public:
  static void registerEvent(TimerEvent* event);
  static void unregisterEvent(TimerEvent* event);
  static void run(mword currentTick);  // should only be called by PIT
};

#endif /* _DynamicTimer_h_ */
