#ifndef _CdiMemoryManager_h_
#define _CdiMemoryManager_h_ 1

#include "cdi/mem.h"

class CdiMemoryManager {
  CdiMemoryManager() = delete;
  CdiMemoryManager(const CdiMemoryManager&) = delete;
  CdiMemoryManager& operator=(const CdiMemoryManager&) = delete;

public:
  static cdi_mem_area* alloc(size_t size, cdi_mem_flags_t flags);
};

#endif /* _CdiMemoryManager_h_ */
