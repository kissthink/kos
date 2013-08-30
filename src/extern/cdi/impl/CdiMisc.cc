/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include "cdi/misc.h"
#include "cdi/printf.h"

// KOS
#include "kern/Debug.h"
#include "kern/Kernel.h"
#include "ipc/SpinLock.h"
#include "mach/IOPort.h"
#include "mach/IOPortManager.h"
#include "mach/IRQManager.h"
#include "mach/Machine.h"
#include "mach/Processor.h"

static const int IRQ_COUNT = 0x10;

static void (*driver_irq_handler[IRQ_COUNT])(cdi_device*) = { nullptr };
static cdi_device* volatile driver_irq_device[IRQ_COUNT] = { nullptr };
static unsigned int irqCount[IRQ_COUNT] = { 0 };
static Thread* sleepingThread[IRQ_COUNT] = { nullptr };
static SpinLock waitIrqLock[IRQ_COUNT];

static void cdiIrqHandler(ptr_t irqPtr) {
  mword irq = *(mword *) irqPtr;
  CdiPrintf("Got CDI interrupt %d\n", irq);
  ScopedLock<> lo(waitIrqLock[irq]);
  if (driver_irq_handler[irq]) {
    driver_irq_handler[irq](driver_irq_device[irq]);
  }
  irqCount[irq] += 1;
  if ( sleepingThread[irq] ) {                // a thread is sleeping for IRQ to occur
    if (!kernelScheduler.cancelTimerEvent(*sleepingThread[irq])) {
      CdiPrintf("IRQ %d already canceled\n", irq);
    }
    sleepingThread[irq] = nullptr;
  }
}

void cdi_register_irq(uint8_t irq, void (*handler)(cdi_device *),
                      cdi_device* device) {
  if (irq >= IRQ_COUNT) {
    CdiPrintf("Registering irq %d\n", irq);
    return;
  }

  KASSERT0( driver_irq_handler[irq] == nullptr );
  driver_irq_handler[irq] = handler;
  driver_irq_device[irq] = device;
  CdiPrintf("Registered device %x at irq %d\n", device, irq);

  IRQManager::registerIRQ(irq, cdiIrqHandler);
}

int cdi_reset_wait_irq(uint8_t irq) {
  if (irq > IRQ_COUNT) return -1;
  ScopedLock<> lo(waitIrqLock[irq]);
  irqCount[irq] = 0;
  return 0;
}

int cdi_wait_irq(uint8_t irq, uint32_t timeout) {
  if (irq >= IRQ_COUNT) {
    CdiPrintf("Waiting for irq %d\n", irq);
    return -1;
  }
  if (driver_irq_handler[irq] == nullptr) {
    CdiPrintf("Handler not set for irq %d\n", irq);
    return -2;
  }
  waitIrqLock[irq].acquire();
  if (irqCount[irq]) {            // IRQ occurred already
    waitIrqLock[irq].release();
    return 0;
  }
  KASSERT0( !sleepingThread[irq] );
  sleepingThread[irq] = Processor::getCurrThread();
  CdiPrintf("Sleeping %x %s\n", sleepingThread[irq], sleepingThread[irq]->getName());
  if (kernelScheduler.sleep(Machine::now() + timeout, waitIrqLock[irq])) {
    return -3;  // timedout
  }
  return 0;     // interrupted
}

int cdi_ioports_alloc(uint16_t start, uint16_t count) {
  IOPort ioPort("cdi ioport");
  bool success = IOPortManager::allocate(ioPort, start, count);
  return success ? 0 : -1;
}

int cdi_ioports_free(uint16_t start, uint16_t count) {
  bool success = IOPortManager::release(start, count);
  return success ? 0 : -1;
}

void cdi_sleep_ms(uint32_t ms) {
  KASSERT0( Processor::interruptsEnabled() );
  kernelScheduler.sleep(ms);
}
