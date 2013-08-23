#include "cdi/mem.h"
#include "impl/CdiMemoryManager.h"

cdi_mem_area* cdi_mem_alloc(size_t size, cdi_mem_flags_t flags) {
  return CdiMemoryManager::alloc(size, flags);
}
