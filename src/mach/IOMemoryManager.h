#ifndef _IOMemoryManager_h_
#define _IOMemoryManager_h_ 1

#include "mach/IOMemory.h"
#include "util/basics.h"

class IOMemoryManager {
  IOMemoryManager() = delete;
  IOMemoryManager(const IOMemoryManager&) = delete;
  IOMemoryManager& operator=(const IOMemoryManager&) = delete;

public:
  static const BitSeg<uint32_t, 0, 1> Under1MB;
  static const BitSeg<uint32_t, 1, 1> Prefetchable;
  static const BitSeg<uint32_t, 2, 1> Any32Bit;

private:
  static const uint32_t mask = Under1MB() | Prefetchable() | Any32Bit();

public:
  static bool allocRegion(IOMemory& mem, laddr baseAddr, size_t range, uint32_t flags);
};

#endif /* _IOMemoryManager_h_ */
