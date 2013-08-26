#include "mach/Device.h"

Device Device::root("Root Device", Device::Root);

void Device::release() {
  while (!ioPorts.empty()) {
    IOPort* port = ioPorts.front();
    ioPorts.pop_front();
    delete port;
  }
  while (!ioMemoryRegions.empty()) {
    IOMemory* mem = ioMemoryRegions.front();
    ioMemoryRegions.pop_front();
    delete mem;
  }
  // delete all children ?
  while (!children.empty()) {
    Device* dev = children.front();
    children.pop_front();
    delete dev;
  }
}
