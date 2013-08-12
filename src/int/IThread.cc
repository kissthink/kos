#include "int/IEvent.h"
#include "int/IHandler.h"
#include "int/IThread.h"
#include "kern/Thread.h"

void IThread::executeHandlers() {
  ScopedLock<> lo(event->lk);
  InPlaceSet<IHandler*,0,less<IHandler*>>::iterator it = event->handlers.begin();
  while (it != event->handlers.end()) {
    IHandler* h = *it;
    if (h->pendingRemoval) {      // set to be removed
      event->removeHandler(h);
//      h->broadcast();             // wake up all threads waiting for the handler to be removed
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
    ++it;
  }
}
