#ifndef _ISource_h_
#define _ISource_h_ 1

#include "mach/platform.h"

class IEvent;
class Interrupt;

class ISource {
  friend void Interrupt::registerSource(uint32_t, uint32_t, laddr);
  IEvent* event;
  laddr ioApicAddr;
  int vector;
  int irq;
  ISource(laddr addr, int v, int i) : event(nullptr), ioApicAddr(addr), vector(v), irq(i) {}
public:
  laddr getIoApicAddr() {
    return ioApicAddr;
  }
  int getVector() {
    return vector;
  }
  int getIRQ() {
    return irq;
  }
  void maskIRQ() {
    // TODO
  }
  IEvent* getEvent() {
    return event;
  }
};

#endif /* _ISource_h_ */
