#ifndef _Interrupt_h_
#define _Interrupt_h_ 1

#include "ipc/SpinLock.h"
#include "mach/platform.h"
#include "util/basics.h"
#include "extern/stl/mod_list"

class IEvent;
class IHandler;
class ISource;

enum FilerReturn {
  FILTER_SCHEDULE_THREAD
};

typedef void preIThreadFunc(ptr_t);
typedef void postIThreadFunc(ptr_t);
typedef void postFilterFunc(ptr_t);

class Interrupt {
  static const int APIC_NUM_IOINTS = 191;
  static const int APIC_IO_INTS = 48;
  static const int NUM_IO_INTS = 768;
  static ISource* sources[NUM_IO_INTS];   // look for interrupt source with idt vector value
  static InPlaceList<IEvent*> eventList;
  static int LAPIC_IRQS[APIC_NUM_IOINTS+1];
  static SpinLock lk;

  static uint32_t allocateVector(uint32_t irq);   // use to find IOAPIC irq for given vector later

  Interrupt() = delete;
  Interrupt(const Interrupt&) = delete;
  const Interrupt& operator=(const Interrupt&) = delete;
public:
  static void init();
  static void registerSource(ISource* source);
  static void registerSource(uint32_t,uint32_t,laddr);
  static ISource* lookupSource(int irq);
  static void addHandler(int vector, IHandler* handler);
  static void removeHandler(IHandler* handler);
  static void disableSource(ISource* source);
  static void runHandlers(ISource* source);
  static void resume();
  static void suspend();
  static void assignCPU(ISource* source, mword apicId);
  static mword nextCPU();
  static void bindCPU(int vector, mword apicId);
  static void shuffleIRQs();

  // kern_intr.c
  static IEvent* createEvent(ISource*, int, preIThreadFunc, postIThreadFunc, postFilterFunc, bool soft);
  static IEvent* lookUp(int irq);
  static void scheduleSWI(IHandler* h);   // schedule software interrupt
};

#endif /* _Interrupt_h_ */
