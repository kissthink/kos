#include "test/SleepQueueTest.h"
#include "ipc/SleepQueue.h"
#include "mach/Processor.h"
#include "kern/Thread.h"
#include <atomic>

static atomic<int> numTestsDone;

static void waiter(ptr_t arg) {
  SleepQueue::sleep(&numTestsDone);
  DBG::outln(DBG::Basic, "woke up");
  numTestsDone += 1;
}

void SleepQueueTest() {
  DBG::outln(DBG::Basic, "running SleepQueueTest...");
  numTestsDone = 0;
  for (int i = 0; i < 10; i++) {
    Thread::create(waiter, nullptr, kernelSpace, "s");
  }
  Processor::getCurrThread()->sleep(5000);
  DBG::outln(DBG::Basic, "waking up all threads...");
  SleepQueue::wakeup(&numTestsDone);
  while (numTestsDone != 10) Pause();
  DBG::outln(DBG::Basic, "SleepQueueTest success");
}
