#include "cdi/mem.h"
#include "impl/CdiMemoryManager.h"

cdi_mem_area* cdi_mem_alloc(size_t size, cdi_mem_flags_t flags) {
  return CdiMemoryManager::alloc(size, flags);
}

cdi_mem_area* cdi_mem_map(uintptr_t paddr, size_t size) {
  return CdiMemoryManager::reserve(paddr, size);
}

void cdi_mem_free(cdi_mem_area* p) {
  CdiMemoryManager::free(p);
}