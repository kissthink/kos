#include <cstdint>
#include "TimerTest.h"
#include "kern/DynamicTimer.h"
#include "kern/Debug.h"
#include "ipc/BlockingSync.h"
#include "mach/Delay.h"
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
  for (int i = 0; i < 10; i++) {
    Thread::create(test1, new mword(i*500+1), kernelSpace, "t");
  }
  while (numTestsDone < 10) Pause();
  DBG::outln(DBG::Basic, "TimerTest success");
}

void DelayTest() {
  DBG::outln(DBG::Basic, "running DelayTest...");
  DBG::outln(DBG::Basic, "delay for 100ns");
  mword start = Machine::now();
  Delay::ndelay(100);
  mword end = Machine::now();
  DBG::outln(DBG::Basic, "100ns took ", end-start, "cycles");
  DBG::outln(DBG::Basic, "done.");
  DBG::outln(DBG::Basic, "delay for 100us");
  start = Machine::now();
  Delay::udelay(100);
  end = Machine::now();
  DBG::outln(DBG::Basic, "100us took ", end-start, "cycles");
  DBG::outln(DBG::Basic, "done.");
  for (int i = 0; i < 5; i++) {
    DBG::outln(DBG::Basic, "delay for 2000us");
    start = Machine::now();
    Delay::udelay(2000);
    end = Machine::now();
    DBG::outln(DBG::Basic, "2000us took ", end-start, "cycles");
    DBG::outln(DBG::Basic, "done.");
  }
}
