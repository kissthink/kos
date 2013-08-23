#include "mach/IOPortManager.h"

BuddyMap<0,12,IOPortManager::BuddySet> IOPortManager::availablePortRange;
SpinLock IOPortManager::lk;
bool IOPortManager::initialized = false;

// initializes internal buddy map
void IOPortManager::init() {
  ScopedLock<> lo(lk);
  if (!initialized) {
    uint16_t start = 0;
    size_t size = 0x1000; // hope this is enough
    availablePortRange.insert(start, size);
    DBG::outln(DBG::Devices, "IOPort reserve: ", FmtHex(start), '/', FmtHex(size));
    initialized = true;
  }
}

// reserves a specific port address for device I/O operations
bool IOPortManager::allocate(IOPort& ioPort, uint16_t port, size_t size) {
  ScopedLock<> lo(lk);
  KASSERT0(initialized);
  bool reserved = availablePortRange.remove( port, size );
  if (reserved) {
    DBG::outln(DBG::Devices, "IOPort alloc: ", FmtHex(size), " -> ", FmtHex(port));
    ioPort.baseAddr = port;
    ioPort.portRange = size;
  }
  return reserved;
}

// allocates any I/O port for device I/O operations
bool IOPortManager::allocate(IOPort& ioPort, size_t size) {
  ScopedLock<> lo(lk);
  KASSERT0(initialized);
  uint16_t addr = availablePortRange.retrieve<false>( size, 0x2000 );
  if unlikely( addr == 0x2000 ) return false;
  DBG::outln(DBG::Devices, "IOPort alloc: ", FmtHex(size), " -> ", FmtHex(addr));
  ioPort.baseAddr = addr;
  ioPort.portRange = size;
  return true;
}

// make this I/O port address range available
bool IOPortManager::release(IOPort& ioPort) {
  ScopedLock<> lo(lk);
  KASSERT0(initialized);
  return availablePortRange.insert(ioPort.base(), ioPort.range());
}

bool IOPortManager::release(uint16_t base, size_t range) {
  ScopedLock<> lo(lk);
  KASSERT0(initialized);
  return availablePortRange.insert(base, range);
}
