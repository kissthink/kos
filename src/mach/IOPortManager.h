#ifndef _IOPortManager_h_
#define _IOPortManager_h_ 1

#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include "mach/IOPort.h"
#include "mach/platform.h"
#include "util/BuddyMap.h"

#include <set>

class IOPort;
class IOPortManager {
  IOPortManager() = delete;
  IOPortManager(const IOPortManager&) = delete;
  IOPortManager& operator=(const IOPortManager&) = delete;

  using BuddySet = set<uint16_t,less<uint16_t>,KernelAllocator<uint16_t>>;
  static BuddyMap<0,18,BuddySet> availablePortRange;  // 0~64kb reserved
  static SpinLock lk;
  static bool initialized;

public:
  static void init(uint16_t, size_t);
  static bool allocate(IOPort& ioPort, uint16_t port, size_t size); 
  static bool allocate(IOPort& ioPort, size_t size);
  static bool release(IOPort& ioPort);
  static bool release(uint16_t port, size_t size);
};

#endif /* _IOPortManager_h_ */
