#ifndef _AllocationHelper_h_
#define _AllocationHelper_h_ 1

#include "mach/MemoryRegion.h"
#include "mach/PageManager.h"
#include "kern/Kernel.h"
#include "ipc/SpinLock.h"

class FrameManager;

class  AllocationHelper {
  AllocationHelper() = delete;
  AllocationHelper(const AllocationHelper&) = delete;
  AllocationHelper& operator=(const AllocationHelper&) = delete;

  using PageOwner = PageManager::Owner;
  using PageType = PageManager::PageType;

  static SpinLock lk;
public:
  enum Constraints {
    None        = 0,        // no constraints
    Continuous  = 1 << 0,   // physically continous
    NonRAM      = 1 << 1,   // allocate pages that are not in RAM
    Force       = 1 << 2,   // remove range but if that fails, still map physical region
    Below1MB    = 1 << 3,   // allocate pages below 1MB
    Below16MB   = 1 << 4,   // allocate pages below 16MB
    Below4GB    = 1 << 5,   // allocate below 4GB
    Below64GB   = 1 << 6,   // allocate below 64GB
  };

  static void init();
  static bool allocRegion(MemoryRegion& region, size_t pages, Constraints constraints, PageOwner owner, PageType type);
  static bool allocRegion(MemoryRegion& region, size_t pages, laddr start, Constraints constraints, PageOwner owner, PageType type);
};

inline AllocationHelper::Constraints operator|(AllocationHelper::Constraints a, AllocationHelper::Constraints b) {
  return static_cast<AllocationHelper::Constraints>(static_cast<int>(a) | static_cast<int>(b));
}


#endif /* _AllocationHelper_h_ */
