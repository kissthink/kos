#ifndef _IOPortManager_h_
#define _IOPortManager_h_ 1

#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include "mach/IOPort.h"
#include "mach/platform.h"
#include "util/RangeList.h"
#include <list>

class IOPortManager {
  IOPortManager() = delete;
  IOPortManager(const IOPortManager&) = delete;
  IOPortManager& operator=(const IOPortManager&) = delete;

public:
  struct IOPortInfo {
    IOPortInfo(uint16_t port, mword size, const char* name)
      : port(port), size(size), name(name) {}

    uint16_t port;
    mword size;
    const char* name;
  };

private:
  static SpinLock lk;
  static RangeList<uint32_t> freePorts;
  static std::list<IOPort*,KernelAllocator<IOPort*>> usedPorts;

public:
  static bool allocate(IOPort* ioPort, uint16_t port, mword size);
  static void free(IOPort* port);
  static void allocateIOPortList(std::list<IOPortInfo*,KernelAllocator<IOPortInfo*>>& ports);
  static void freeIOPortList(std::list<IOPortInfo*,KernelAllocator<IOPortInfo*>>& ports);
  static void init(uint16_t base, mword size);
};

#endif /* _IOPortManager_h_ */
