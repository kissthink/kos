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
Mutex DynamicTimer::mtx;
ConditionVar DynamicTimer::condVar;
set<TimerEntry*,DynamicTimerCompare,KernelAllocator<TimerEntry*>>  DynamicTimer::entries;
Thread* DynamicTimer::thread;

void DynamicTimer::init() {
  thread = Thread::create(timerLoop, nullptr, kernelSpace, "TIMER");
}

bool DynamicTimer::cancel(TimerEntry* entry) {
  ScopedLock<> lo(lk);
  TimerSet::iterator it = entries.find(entry);
  if (it == entries.end()) return false;    // already completed
  if ((*it)->running) return false;         // currently running
  return entries.erase(it) != entries.end();
}

bool DynamicTimer::cancelSync(TimerEntry* entry) {
  ScopedLock<> lo(lk);
  TimerSet::iterator it = entries.find(entry);
  if (it == entries.end()) return false;    // already completed
  if ((*it)->running) {
    mtx.acquire();
    while (entries.find(entry) != entries.end())  // wait till it completes
      condVar.wait(&mtx);
    mtx.release();
    return false;
  }
  return entries.erase(entry);
}

// TODO implement
bool DynamicTimer::resetEntry(TimerEntry* oldEntry, TimerEntry* newEntry) {
  return true;
}

void DynamicTimer::run() {
  Processor::incLockCount();    // disable preemption
  mword now = Machine::now();
  for (;;) {
    ScopedLock<> lo(lk);
    TimerSet::iterator i = entries.begin();
  if (i == entries.end() || now < (*i)->time) break;
    (*i)->running = true;
    TimerEntry* curEntry = *i;
    lk.release();
    curEntry->func(curEntry->arg);
    lk.acquire();
    mtx.acquire();
    entries.erase(curEntry);
    condVar.signal();
  }
  Processor::decLockCount();    // restore preemption
}
