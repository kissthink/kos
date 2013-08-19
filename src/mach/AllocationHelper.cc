#include "mach/AllocationHelper.h"
#include "ipc/SpinLock.h"
#include "kern/Kernel.h"

static SpinLock lk;

static const size_t continuous = 1 << 0;
static const size_t nonRamMemory = 1 << 1;
static const size_t force = 1 << 2;

bool allocateRegion(MemoryRegion& region, size_t pages, size_t constraints, size_t flags) {
  ScopedLock<> lo(lk);
  vaddr vAddress;
  if (kernelSpace.allocPages<1>(pagesize<1>() * pages, AddressSpace::Data)) {
    region.virtualAddr = reinterpret_cast<ptr_t>(vAddress);
    region.physicalAddr = 0;
    region.regSize = pages * pagesize<1>();
    return true;
  }
  return false;
}

