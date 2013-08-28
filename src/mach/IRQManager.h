#ifndef _IRQManager_h_
#define _IRQManager_h_ 1

#include "kern/Kernel.h"
#include "kern/KernelQueues.h"
#include "kern/Thread.h"
#include "ipc/SpinLock.h"
#include "ipc/SyncQueues.h"
#include "mach/platform.h"
#include "mach/Machine.h"
#include "util/basics.h"

#include <list>

class IRQManager {
  IRQManager() = delete;
  IRQManager(const IRQManager&) = delete;
  IRQManager& operator=(const IRQManager&) = delete;

  static uint32_t* irqToVector;
  static int vectorToIRQ[256];
  using IRQHandlers = list<function_t,KernelAllocator<function_t>>;
  static IRQHandlers* irqHandlers;  // handlers per IRQ
  static SpinLock* lk;              // locks per IRQ
  static bool initialized;
  static unsigned int numIRQ;

  static Thread* softIRQThread;     // bottom half
  static ProdConsQueue<StaticRingBuffer<mword, 128>> irqQueue;
  static void softIRQ(ptr_t arg);

  static void dump();
public:
  static void init(int rdr);    // pass # of IRQ supported by I/O APIC
  static void registerIRQ(unsigned int irq, function_t handler);
  static bool handleIRQ(mword vector);
};

#endif /* _IRQManager_h_ */
