#ifndef _AllocationHelper_h_
#define _AllocationHelper_h_ 1

#include "mach/MemoryRegion.h"
#include "kern/Kernel.h"

bool allocateRegion(MemoryRegion& region, size_t pages, size_t constraints, size_t flags);

#endif /* _AllocationHelper_h_ */
