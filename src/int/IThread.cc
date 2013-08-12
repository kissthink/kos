#include "int/IEvent.h"
#include "int/IHandler.h"
#include "int/IThread.h"
#include "ipc/SleepQueue.h"
#include "kern/Thread.h"

void IThread::executeHandlers() {
  ScopedLock<> lo(event->lk);
  InPlaceSet<IHandler*,0,less<IHandler*>>::iterator it = event->handlers.begin();
  while (it != event->handlers.end()) {
    IHandler* h = *it;
    h->run = true;
    if (h->pendingRemoval) {      // set to be removed
      event->removeHandler(h);
      SleepQueue::wakeup(h);      // wake up all threads waiting for the handler to be removed
      continue;
    }
    if (!h->handler) continue;    // only run handlers in ithread
    if (event->isSoftInterrupt()) {
      if (!h->need)
        continue;
      else
        h->need = false;
    }
    h->handler(h->arg);   // run!
    h->run = false;
    ++it;
  }

  if (event->postIThread) {
    event->postIThread(event->source);
  }
}

void ithreadLoop(ptr_t arg) {
  IThread* it = (IThread*) arg;
  DBG::outln(DBG::Basic, "ithread:", FmtHex(it));
  for (;;) {
    while (it->need) {
      it->need = false;
      it->executeHandlers();
    }
    kernelScheduler.suspend();
    DBG::outln(DBG::Basic, "ithreadLoop resume");
  }
}

void IThread::schedule() {
  KASSERT1(thread, "null thread");
  need = true;
  DBG::outln(DBG::Basic, "scheduling...");
  static bool firstTime = true;
  if (firstTime) {
    thread->run(threadLoop, this);
    firstTime = false;
  } else {
    kernelScheduler.start(*thread);
  }
}
