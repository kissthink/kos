#include "int/IEvent.h"
#include "int/IHandler.h"
#include "int/ISource.h"
#include "int/IThread.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"

void IEvent::addHandler(IHandler* h) {
  ScopedLock<> lo(lk);
  handlers.insert(h);
}

void IEvent::removeHandler(IHandler* h) {
  ScopedLock<> lo(lk);
  handlers.erase(h);
}

void IEvent::createIThread(const char* name) {
  ScopedLock<> lo(lk);
  if (!ithread) {
    Thread* t = Thread::create(kernelSpace, name);
    ithread = new IThread(t, this);
  }
}

int IEvent::getVector() {
  return source->getVector();
}

int IEvent::getIRQ() {
  return source->getIRQ();
}
