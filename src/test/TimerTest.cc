#include <cstdint>
#include "TimerTest.h"
#include "kern/DynamicTimer.h"
#include "kern/Debug.h"
#include "ipc/BlockingSync.h"
#include <atomic>

static atomic<int> numTestsDone;
static atomic<int> counter;
static Semaphore sem(1);

void increment(ptr_t) {
  counter += 1;
}

static void test1(ptr_t arg) {
  sem.P();
  mword startTime = Machine::now();
  mword timeout = *(mword *)arg;
  DBG::outln(DBG::Basic, "test for timeout ", timeout, "ms");
  TimerEntry* entry = new TimerEntry(increment, nullptr, timeout);
  DynamicTimer::addEvent(entry);
  while (counter != 1) Pause();
  KASSERT1(Machine::now()-startTime >= timeout, "callback ran early");
  counter = 0;
  numTestsDone += 1;
  sem.V();
}

void TimerTest() {
  DBG::outln(DBG::Basic, "running TimerTest...");
  numTestsDone = counter = 0;
  for (int i = 0; i < 20; i++) {
    Thread::create(test1, new mword(i*500+1), kernelSpace, "t");
  }
  while (numTestsDone < 20) Pause();
  DBG::outln(DBG::Basic, "TimerTest success");
}
