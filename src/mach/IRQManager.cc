#include "mach/IRQManager.h"
#include "kern/Debug.h"

// IRQ assignment is from http://www.osdever.net/tutorials/pdf/apic.pdf
uint32_t* IRQManager::irqToVector;
int IRQManager::vectorToIRQ[256] = { -1 };
IRQManager::IRQHandlers* IRQManager::irqHandlers;
SpinLock* IRQManager::lk;
bool IRQManager::initialized = false;
unsigned int IRQManager::numIRQ = 0;

Thread* IRQManager::softIRQThread[4];
//ProdConsQueue<StaticRingBuffer<mword, 256>> IRQManager::irqQueue;
//IRQProducerConsumerQueue IRQManager::irqQueue(256);
SimpleProdConsQueue<mword> IRQManager::irqQueue(256);

// priority (highest->lowest) to IRQ number
static int defaultIRQPriorities[16] = { 0,1,2,11,12,13,14,15,3,4,5,6,7,8,9,10 };

// bottom-half thread handling queued IRQs
void IRQManager::softIRQ(ptr_t arg) {
  KASSERT0( initialized );
  for (;;) {
    mword irq = irqQueue.remove();  // sleep until interrupt occurs
    DBG::outln(DBG::Basic, "IRQ ", irq, " occurred!");
    ScopedLock<> lo(lk[irq]);
    for (function_t f : irqHandlers[irq]) {
      (*f)(&irq);
    }
//    DBG::outln(DBG::Basic, "IRQ ", irq, " done!");
  }
}

// map I/O APIC IRQ entries to IDT vector numbers
void IRQManager::init(int rdr) {
  irqToVector = new uint32_t[rdr];
  for (int i = 0; i < rdr; i++) {
    if (i < 16) {
      irqToVector[i] = 0xec - defaultIRQPriorities[i] * 8;
      vectorToIRQ[0xec-defaultIRQPriorities[i]*8] = i;
    } else {
      irqToVector[i] = 0x6c-(i-16)*8;
      vectorToIRQ[0x6c-(i-16)*8] = i;
    }
  }
  irqHandlers = new IRQHandlers[rdr];
  lk = new SpinLock[rdr];
  numIRQ = rdr;
  initialized = true;

  softIRQThread[0] = Thread::create(kernelSpace, "IRQ thread1");
  softIRQThread[1] = Thread::create(kernelSpace, "IRQ thread2");
  softIRQThread[2] = Thread::create(kernelSpace, "IRQ thread3");
  softIRQThread[3] = Thread::create(kernelSpace, "IRQ thread4");
//  softIRQThread->setPriority(1);
  kernelScheduler.run(*softIRQThread[0], softIRQ, nullptr);      // start handling IRQ
  kernelScheduler.run(*softIRQThread[1], softIRQ, nullptr);      // start handling IRQ
  kernelScheduler.run(*softIRQThread[2], softIRQ, nullptr);      // start handling IRQ
  kernelScheduler.run(*softIRQThread[3], softIRQ, nullptr);      // start handling IRQ

  dump();
}

void IRQManager::registerIRQ(unsigned int irq, function_t handler) {
  KASSERT0( initialized );
  KASSERT1( irq < numIRQ, irq );
  
  ScopedLock<> lo(lk[irq]);
  if (irqHandlers[irq].empty()) {
    DBG::outln(DBG::Basic, "Mapping irq: ", FmtHex(irq));
    Machine::staticEnableIRQ( irq, irqToVector[irq] );
  }
  irqHandlers[irq].push_back(handler);
  DBG::outln(DBG::Basic, "registered IRQ handler: ", FmtHex(ptr_t(handler)), " for IRQ ", irq);
}

// queue IRQ before returning from interrupt handler
bool IRQManager::handleIRQ(mword vector) {
  KASSERT0( initialized );
  int irq = vectorToIRQ[vector];
  if (irq == -1) return false;
  while (!irqQueue.tryAppend(irq)) Pause();
//  irqQueue.append(irq);
  return true;
}

void IRQManager::dump() {
  KASSERT0( initialized );
  for (unsigned int i = 0; i < numIRQ; i++) {
    DBG::outln(DBG::Basic, "IRQ to Vector: ", FmtHex(i), " -> ", FmtHex(irqToVector[i]));
  }
  for (int i = 0; i < 256; i++) {
    if (vectorToIRQ[i] != -1) {
      DBG::outln(DBG::Basic, "Vector to IRQ: ", FmtHex(i), " -> ", FmtHex(vectorToIRQ[i]));
    }
  }
}
