#include "mach/AllocationHelper.h"
#include "kern/Kernel.h"
#include "kern/FrameManager.h"

#define CONST_1MB   0x100000    // 1048576
#define CONST_16MB  0x1000000   // 16777216

SpinLock AllocationHelper::lk;

bool AllocationHelper::allocRegion(MemoryRegion& region, size_t pages, Constraints con,
                                   PageOwner owner, PageType type) {
  ScopedLock<> lo(lk);
  if (!Processor::getFrameManager()) {
    ABORT1("FrameManager is not initialized yet");
  }

  // for continuous memory, use address below 16MB
  if (con & Continuous) {
    if (!(con & Below1MB) && !(con & Below16MB)) {
      con = con|Below16MB;
    }
  }
  // allocate physical address range
  laddr physicalAddr = 0;
  if (con & Below1MB) {
    physicalAddr = Processor::getFrameManager()->alloc<true>(pages * pagesize<1>(), CONST_1MB);
    DBG::outln(DBG::VM, "allocRegion() allocated below 1MB phyAddr: ", FmtHex(physicalAddr));
  } else if (con & Below16MB) {
    physicalAddr = Processor::getFrameManager()->alloc<true>(pages * pagesize<1>(), CONST_16MB);
    DBG::outln(DBG::VM, "allocRegion() allocated below 16MB phyAddr: ", FmtHex(physicalAddr));
  } else {
    physicalAddr = Processor::getFrameManager()->alloc<true>(pages * pagesize<1>());
    DBG::outln(DBG::VM, "allocRegion() allocated phyAddr: ", FmtHex(physicalAddr));
  }

  if (physicalAddr == 0) {
    DBG::outln(DBG::VM, "allocRegion() failed allocating address");
    return false;
  }
  if (owner == PageManager::User) {
    ABORT1("user space allocation not implemented yet");
  }
  vaddr vAddress = kernelSpace.mapPages<1>(physicalAddr, pages * pagesize<1>(), type);
  if (vAddress) {
    DBG::outln(DBG::VM, "allocRegion() mapped to virtual addr: ", FmtHex(vAddress));
    region.virtualAddr = reinterpret_cast<ptr_t>(vAddress);
    region.physicalAddr = physicalAddr;
    region.regSize = pages * pagesize<1>();
    return true;
  }
  return false;
}

bool AllocationHelper::allocRegion(MemoryRegion& region, size_t pages, laddr start, Constraints con,
                                   PageOwner owner, PageType type) {
  ScopedLock<> lo(lk);
  if (!Processor::getFrameManager()) {
    ABORT1("FrameManager is not initialized yet");
  }
  if (!(con & Continuous)) {
    ABORT1("allocRegion() cannot allocate non-contiguous physical region");
  }
  if (con & NonRAM) {
    region.setNonRamMemory(true);
    bool reserved = Processor::getFrameManager()->reserve(start, pages * pagesize<1>());
    if (!reserved) {
      if (!(con & Force)) {
        return false;
      }
      region.setForced(true);
    }
  } else {
    if (start >= CONST_16MB) {
      region.setNonRamMemory(true); // do not free()
      region.setForced(true);
    }
  }
  if (owner == PageManager::User) {
    ABORT1("user space allocation not implemented yet");
  }
  vaddr vAddress = kernelSpace.mapPages<1>(start, pages * pagesize<1>(), type);
  if (vAddress) {
    DBG::outln(DBG::VM, "allocRegion() mapped ", FmtHex(start), " to virtual addr: ", FmtHex(vAddress));
    region.virtualAddr = reinterpret_cast<ptr_t>(vAddress);
    region.physicalAddr = start;
    region.regSize = pages * pagesize<1>();
    return true;
  }
  return false;
}
