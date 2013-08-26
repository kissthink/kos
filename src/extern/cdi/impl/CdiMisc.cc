#include "cdi/misc.h"

// KOS
#include "kern/Debug.h"
#include "kern/Kernel.h"
#include "mach/IOPort.h"
#include "mach/IOPortManager.h"
#include "mach/IRQManager.h"
#include "mach/Machine.h"
#include "mach/Processor.h"

static const int IRQ_COUNT = 0x10;

static void (*driver_irq_handler[IRQ_COUNT])(cdi_device*) = { nullptr };
static cdi_device* driver_irq_device[IRQ_COUNT] = { nullptr };
static bool irqWaiting[IRQ_COUNT] = { false };

static void cdiIrqHandler(ptr_t irqPtr) {
  mword* irq = (mword *) irqPtr;
  DBG::outln(DBG::CDI, "Got cdi interrupt: ", FmtHex(*irq));
  if (driver_irq_handler[*irq]) {
    driver_irq_handler[*irq](driver_irq_device[*irq]);
  }
  if (irqWaiting[*irq]) {    // cdi_wait_irq called
    if (!kernelScheduler.cancelTimerEvent(*Processor::getCurrThread())) {
      DBG::outln(DBG::CDI, "IRQ ", FmtHex(*irq), " already canceled");
    }
  }
  delete irq;
}

void cdi_register_irq(uint8_t irq, void (*handler)(cdi_device *),
                      cdi_device* device) {
  if (irq >= IRQ_COUNT) {
    DBG::outln(DBG::CDI, "cdi_register_irq() called with irq: ", FmtHex(irq));
    return;
  }

  KASSERT0( driver_irq_handler[irq] == nullptr );
  driver_irq_handler[irq] = handler;
  driver_irq_device[irq] = device;

  IRQManager::registerIRQ(irq, cdiIrqHandler);
}

int cdi_reset_wait_irq(uint8_t irq) {
  if (irq > IRQ_COUNT) return -1;
  irqWaiting[irq] = false;
  return 0;
}

int cdi_wait_irq(uint8_t irq, uint32_t timeout) {
  if (irq >= IRQ_COUNT) {
    DBG::outln(DBG::CDI, "cdi_wait_irq() called with irq: ", FmtHex(irq));
    return -1;
  }
  if (driver_irq_handler[irq] == nullptr) {
    DBG::outln(DBG::CDI, "cdi_wait_irq() handler not set for irq: ", FmtHex(irq));
    return -2;
  }
  if (irqWaiting[irq]) return 0;
  irqWaiting[irq] = true;
  if (kernelScheduler.sleep(Machine::now() + timeout)) {
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
  kernelScheduler.sleep(ms);
}
