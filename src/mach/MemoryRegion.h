#ifndef _MemoryRegion_h_
#define _MemoryRegion_h_

#include "kern/FrameManager.h"
#include "kern/Kernel.h"
#include "mach/platform.h"

class MemoryRegion {
  friend class AllocationHelper;
  MemoryRegion(const MemoryRegion&) = delete;
  MemoryRegion& operator=(const MemoryRegion&) = delete;
  ptr_t virtualAddr;
  laddr physicalAddr;
  mword regSize; // size of memory-region in bytes
  const char* regName;
  bool isNonRamMemory;
  bool isForced;

public:
  MemoryRegion(const char* name) : virtualAddr(0), physicalAddr(0), regSize(0), regName(name), isNonRamMemory(false), isForced(false) {}
  virtual ~MemoryRegion() { kernelSpace.unmapPages<1>(vaddr(virtualAddr), regSize); }
  void free() {
    kernelSpace.unmapPages<1>(vaddr(virtualAddr), regSize);
  }
  ptr_t virtualAddress() const {
    return virtualAddr;
  }
  laddr physicalAddress() const {
    return physicalAddr;
  }
  mword size() const {
    return regSize;
  }
  const char* name() const {
    return regName;
  }
  operator bool() const {
    return regSize != 0;
  }
  bool physicalBoundsCheck(laddr addr) {
    if (addr >= physicalAddr && addr < (physicalAddr + regSize))
      return true;
    return false;
  }
  template <typename T>
  T* convertPhysicalPointer(laddr addr) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(virtualAddr)
        + (addr - physicalAddr));
  }
  void setNonRamMemory(bool val) {
    isNonRamMemory = val;
  }
  bool getNonRamMemory() {
    return isNonRamMemory;
  }
  void setForced(bool val) {
    isForced = val;
  }
  bool getForced() {
    return isForced;
  }
};

#endif /* _MemoryRegion_h_ */
