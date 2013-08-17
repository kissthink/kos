#include "mach/IOPortManager.h"
using namespace std;

typedef list<IOPortManager::IOPortInfo*, KernelAllocator<IOPortManager::IOPortInfo*>> IOPortInfoList;
SpinLock IOPortManager::lk;
RangeList<uint32_t> IOPortManager::freePorts;
list<IOPort*,KernelAllocator<IOPort*>> IOPortManager::usedPorts;

bool IOPortManager::allocate(IOPort* ioPort, uint16_t port, mword size) {
  ScopedLock<> lo(lk);
  if (!freePorts.allocateSpecific(port, size)) return false;
  usedPorts.push_back(ioPort);
  return true;
}

void IOPortManager::free(IOPort* ioPort) {
  ScopedLock<> lo(lk);
  for (IOPort* port : usedPorts) {
    if (port == ioPort) {
      usedPorts.remove(port);
      break;
    }
  }
  freePorts.free(ioPort->base(), ioPort->size());
}

void IOPortManager::allocateIOPortList(IOPortInfoList& ioPorts) {
  ScopedLock<> lo(lk);
  for (IOPort* port : usedPorts) {
    IOPortInfo* pInfo = new IOPortInfo(port->base(), port->size(), port->name());
    ioPorts.push_back(pInfo);
  }
}

void IOPortManager::freeIOPortList(IOPortInfoList& ioPorts) {
  while (!ioPorts.empty()) {
    IOPortInfo* pInfo = ioPorts.front();
    ioPorts.pop_front();
    delete pInfo;
  }
}

void IOPortManager::init(uint16_t base, mword size) {
  freePorts.free(base, size);
}

