#include "test/InterruptTest.h"
#include "int/Interrupt.h"
#include "int/ISource.h"
#include "int/IEvent.h"
#include "int/IHandler.h"
#include "int/IThread.h"
#include <atomic>

static atomic<int> numTestsDone;

static int filter(ptr_t) {
  DBG::outln(DBG::Basic, "running filter...");
  DBG::outln(DBG::Basic, "scheduling ithread...");
  return FILTER_SCHEDULE_THREAD;
}

static void handler(ptr_t) {
  DBG::outln(DBG::Basic, "running handler...");
  numTestsDone += 1;
}

void InterruptTest() {
  DBG::outln(DBG::Basic, "running InterrupTest...");
  IHandler* h = new IHandler(nullptr, handler, nullptr, "Test", true);
  Interrupt::addHandler(0x05, h);
//  asm volatile("int $0x05");
  Interrupt::scheduleSWI(h);
  while (numTestsDone != 1) Pause();
  DBG::outln(DBG::Basic, "InterruptTest success");
}
