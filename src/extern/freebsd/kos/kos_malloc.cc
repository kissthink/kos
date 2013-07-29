#include "kos/kos_kassert.h"
#include "kos/kos_malloc.h"
#include <cstdint>
#include "kern/KernelHeap.h"

#include <map>
using namespace std;

extern KernelHeap kernelHeap;
map<void *, unsigned long> memorySizeMap;         // tracks size of allocated memory (for realloc)

void *kos_malloc(unsigned long size) {
  void* memory =  kernelHeap.malloc(size);
  if (memory) memorySizeMap[memory] = size;
  return memory;
}

void kos_free(void *addr) {
  BSD_KASSERT0(memorySizeMap.count(addr) == 1);   // check memory is in memorySizeMap
  BSD_KASSERT0(memorySizeMap.erase(addr) == 1);   // check it if correctly deleted
  kernelHeap.free(addr);
}

// Get memory size of allocated memory block (for realloc)
unsigned long kos_getMemorySize(void *addr) {
  if (memorySizeMap.count(addr)) {
    return memorySizeMap[addr];
  }
  return 0;
}
