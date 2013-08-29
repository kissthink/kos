#include "cdi/mem.h"
#include "impl/CdiMemoryManager.h"

// allocate physical memory and return mapped virtual address
cdi_mem_area* cdi_mem_alloc(size_t size, cdi_mem_flags_t flags) {
  return CdiMemoryManager::alloc(size, flags);
}

// map physical address to a virtual address
cdi_mem_area* cdi_mem_map(uintptr_t paddr, size_t size) {
  return CdiMemoryManager::mapPhysical(paddr, size);
}

// free allocated memory area
void cdi_mem_free(cdi_mem_area* p) {
  CdiMemoryManager::free(p);
}
