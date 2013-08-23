#ifndef _IOMemory_h_
#define _IOMemory_h_ 1

#include "mach/platform.h"

class IOMemory {
  friend class IOMemoryManager;

  IOMemory(const IOMemory&) = delete;
  IOMemory& operator=(const IOMemory&) = delete;

  laddr basePhyAddr;
  vaddr baseVAddr;
  mword memSize;
  const char* name;

public:
  IOMemory(const char* name) : basePhyAddr(0), baseVAddr(0), memSize(0), name(name) {}
  ~IOMemory() { releaseMe(); }

  laddr physBase() const {
    return basePhyAddr;
  }
  vaddr virtBase() const {
    return baseVAddr;
  }
  mword size() const {
    return memSize;
  }
  const char* getName() const {
    return name;
  }
  void releaseMe();
};

#endif /* _IOMemory_h_ */
