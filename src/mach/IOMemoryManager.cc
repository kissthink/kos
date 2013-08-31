#include "mach/IOMemoryManager.h"
#include "mach/Processor.h"
#include "kern/Kernel.h"

bool IOMemoryManager::allocRegion(IOMemory& mem, laddr baseAddr, size_t range, uint32_t flags) {
  vaddr addr = 0;
  if (range >= 0x200000) {
    addr = kernelSpace.mapPages<2>( baseAddr, range, AddressSpace::Data );
  } else {
    addr = kernelSpace.mapPages<1>( baseAddr, range, AddressSpace::Data );
  }
  mem.basePhyAddr = baseAddr;
  mem.baseVAddr = addr;
  mem.memSize = range;

  return true;
}
