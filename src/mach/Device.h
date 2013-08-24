#ifndef _Device_h_
#define _Device_h_ 1

#include "mach/IOPort.h"
#include "mach/IOMemory.h"
#include "kern/Kernel.h"

class Device {
  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  list<IOPort*,KernelAllocator<IOPort*>> ioPorts;
  list<IOMemory*,KernelAllocator<IOMemory*>> ioMemoryRegions;
  list<Device*,KernelAllocator<Device*>> children;

  Device* parent;     // parent device
  kstring devName;    // device name
  uint8_t irqNum;     // IRQ number
  static Device root; // root device

  void release();   // destroy all I/O regions
public:
  Device(const char* name) : devName(name) {}
  virtual ~Device() { release(); }
  static Device* getRoot() {
    return &root;
  }
  Device* getParent() {
    return parent;
  }
  void setParent(Device* p) {
    parent = p;
  }
  kstring getName() const {
    return devName;
  }
  uint8_t getIrqNum() const {
    return irqNum;
  }
  void setIrqNum(uint8_t irq) {
    irqNum = irq;
  }
  void addChild(Device* d) {
    children.push_back(d);
  }
  Device* getChild(mword n) {
    mword i = 0;
    auto it = children.begin();
    for ( ; it != children.end(); ++it) {
      if (i == n) return *it;
      ++i, ++it;
    }
    return nullptr;
  }
  void removeChild(mword n) {
    mword i = 0;
    auto it = children.begin();
    for ( ; it != children.end(); ++it) {
      if (i == n) {
        children.erase(it);
        break;
      }
      ++i, ++it;
    }
  }
  void removeChild(Device* d) {
    mword i = 0;
    auto it = children.begin();
    for ( ; it != children.end(); ++it) {
      if (*it == d) {
        children.erase(it);
        break;
      }
      ++i, ++it;
    }
  }
};

#endif /* _Device_h_ */
