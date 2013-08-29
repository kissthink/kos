#include "cdi/misc.h"

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
static bool volatile irqWaiting[IRQ_COUNT] = { false };
static Thread* volatile waitingThread[IRQ_COUNT] = { nullptr };
static SpinLock waitIrqLock[IRQ_COUNT];

static void cdiIrqHandler(ptr_t irqPtr) {
  mword irq = *(mword *) irqPtr;
  DBG::outln(DBG::CDI, "Got cdi interrupt: ", FmtHex(irq));
  if (driver_irq_handler[irq]) {
    driver_irq_handler[irq](driver_irq_device[irq]);
  }
  ScopedLock<> lo(waitIrqLock[irq]);
  if ( irqWaiting[irq] ) {    // cdi_wait_irq called
    KASSERT0( waitingThread[irq] );
    if (!kernelScheduler.cancelTimerEvent(*waitingThread[irq])) {
      DBG::outln(DBG::CDI, "IRQ ", FmtHex(irq), " already canceled");
    }
    irqWaiting[irq] = false;
    waitingThread[irq] = nullptr;
  }
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
  DBG::outln(DBG::CDI, "cdi_register_irq() registered device: ", FmtHex(device), " at irq:", FmtHex(irq));

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
  waitIrqLock[irq].acquire();
  if (irqWaiting[irq]) {
    waitIrqLock[irq].release();
    return 0;
  }
  irqWaiting[irq] = true;
  KASSERT0( !waitingThread[irq] );
  waitingThread[irq] = Processor::getCurrThread();
  if (kernelScheduler.sleep(Machine::now() + timeout, waitIrqLock[irq])) {
    DBG::outln(DBG::CDI, "cdi_wait_irq() timed out");
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
