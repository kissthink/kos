#include <cstdint>
#include "kern/DynamicTimer.h"

static void timerLoop(ptr_t) {
  for (;;) {
    DynamicTimer::run();
    if (DynamicTimer::empty()) kernelScheduler.suspend();
    Pause();
  }
}

SpinLock DynamicTimer::lk;
set<TimerEntry*,DynamicTimerCompare,KernelAllocator<TimerEntry*>>  DynamicTimer::entries;
Thread* DynamicTimer::thread;

void DynamicTimer::init() {
  thread = Thread::create(timerLoop, nullptr, kernelSpace, "TIMER");
}

void DynamicTimer::run() {
  ScopedLock<> lo(lk);
  mword now = Machine::now();
  for (;;) {
    TimerSet::iterator i = entries.begin();
  if (i == entries.end() || now < (*i)->time) break;
    (*i)->func((*i)->arg);
    entries.erase(i);
  }
}
