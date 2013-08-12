#include "int/Interrupt.h"
#include "int/IEvent.h"
#include "int/IHandler.h"
#include "int/ISource.h"
#include "int/IThread.h"
#include "kern/Debug.h"

ISource* Interrupt::sources[NUM_IO_INTS];
InPlaceList<IEvent*> Interrupt::eventList;
SpinLock Interrupt::lk;

static void enableSource(ptr_t arg) {
  ISource* src = (ISource *) arg;
  KASSERT1(src, "null source");
  volatile IOAPIC* ioapic = (IOAPIC*)kernelSpace.mapPages<1>(src->getIoApicAddr(), pagesize<1>(), AddressSpace::Data);
  // unmask irq
  kernelSpace.unmapPages<1>( (vaddr)ioapic, pagesize<1>() );
}

static void disableSource(ptr_t arg) {
  ISource* src = (ISource *) arg;
  KASSERT1(src, "null source");
  volatile IOAPIC* ioapic = (IOAPIC*)kernelSpace.mapPages<1>(src->getIoApicAddr(), pagesize<1>(), AddressSpace::Data);
  ioapic->maskIRQ(src->getIRQ());
  kernelSpace.unmapPages<1>( (vaddr)ioapic, pagesize<1>() );
  Processor::sendEOI();
}

static void sendEOI(ptr_t arg) {
  Processor::sendEOI();
}

static void assignCPU(ptr_t arg) {
  // TODO
}

void Interrupt::init() {
  ScopedLock<> lo(lk);
  for (int i = 0; i < NUM_IO_INTS; i++) {
    sources[i] = nullptr;
  }
}

void Interrupt::registerSource(ISource* source) {
  KASSERT1(source, "null source");
  ScopedLock<> lo(lk);
  if (sources[source->getVector()]) return;
  sources[source->getVector()] = source;
}

void Interrupt::registerSource(uint32_t irq, uint32_t pin, laddr ioApicAddr) {
  ScopedLock<> lo(lk);
  if (!sources[irq]) {
    ISource* source = new ISource(ioApicAddr, pin, irq);
    IEvent* event = new IEvent(source, ::disableSource, ::enableSource, ::sendEOI, false);
    source->event = event;
    sources[irq] = source;
    DBG::outln(DBG::Basic, "registered:", FmtHex(irq));
  }
}

ISource* Interrupt::lookupSource(int vector) {
  ScopedLock<> lo(lk);
  return sources[vector];
}

// add an interrupt handler for an interrupt vector
// also create ithread if the handler requires thread context to work
void Interrupt::addHandler(int vector, IHandler* handler) {
  KASSERT1(vector, vector);
  KASSERT1(handler, "null handler");
  ISource* s = sources[vector];         // vector must be from registered interrupt source
  KASSERT1(s, "source is not registered");
  IEvent* e = s->getEvent();
  handler->setEvent(e);
  bool added = e->addHandler(handler);  // checks for exclusive mode
  if (added) {
    if (handler->hasHandler()) {
      e->createIThread("");
    }
  }
}

void Interrupt::removeHandler(IHandler* handler) {
  KASSERT1(handler, "null handler");
  KASSERT1(handler->event, "null event");

  // you can safely remove handler if there isn't an ithread
  if (!handler->event->ithread) {
    handler->event->removeHandler(handler);
    return;
  }
  // if handler is running, mark it as being dead and let ithread do work
  if (handler->need || handler->run) {
    handler->remove();    // wait until it is actually removed
    SleepQueue::sleep(handler);
  } else {
    handler->event->removeHandler(handler);
  }
}

// mask the interrupt pin and send EOI
void Interrupt::disableSource(ISource* source) {
  KASSERT1(source, "null source");
  source->maskIRQ();
  Processor::sendEOI();
}

void Interrupt::runHandlers(ISource* source) {
  KASSERT1(source, "null source");
  IEvent* e = source->getEvent();
  KASSERT1(e, "null event");
  bool schedule = false;
  for (IHandler* h : e->handlers) {
    if (h->filter == nullptr) {
      schedule = true;    // run ithread later
      continue;
    }
    int error = h->filter(h->arg);
    if (error == FILTER_SCHEDULE_THREAD) {
      schedule = true;
    }
  }
  if (schedule) {
    e->ithread->schedule();
  }
}

void Interrupt::resume() {
  // TODO
}

void Interrupt::suspend() {
  // TODO
}

void Interrupt::assignCPU(ISource* source, mword apicId) {
  // TODO
}

mword Interrupt::nextCPU() {
  // TODO round robin
  return Processor::getApicID();
}

void Interrupt::bindCPU(int vector, mword apicId) {
  // TODO
}

void Interrupt::shuffleIRQs() {
  // TODO
}

void Interrupt::createEvent(ISource* source, int irq, preIThreadFunc f1 = nullptr,
                            postIThreadFunc f2 = nullptr, postFilterFunc f3 = nullptr, bool soft=false) {
  IEvent* e = new IEvent(source, f1, f2, f3, soft);
  ScopedLock<> lo(lk);
  eventList.push_back(e);
}

// looks for hardware interrupt event with registered handlers for an IRQ
IEvent* Interrupt::lookUp(int irq) {
  ScopedLock<> lo(lk);
  for (IEvent* e : eventList) {
    if (e->getIRQ() == irq &&
      !e->isSoftInterrupt() && e->hasHandlers()) return e;
  }
  return nullptr;
}

void Interrupt::scheduleSWI(IHandler* h) {
  KASSERT1(h, "null handler");
  IEvent* e = h->event;
  KASSERT1(e, "null event");
  KASSERT1(!e->handlers.empty(), "no handlers are registered");
  e->ithread->schedule();
}
