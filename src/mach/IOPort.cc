#include "mach/IOPort.h"
#include "mach/IOPortManager.h"

void IOPort::releaseMe() {
  IOPortManager::release( baseAddr, portRange );
  initialized = false;
}
