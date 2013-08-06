#include "kos/Kassert.h"
#include "kos/Malloc.h"

// KOS
#include <cstdint>
#include "kern/KernelHeap.h"

#include <map>
using namespace std;

extern KernelHeap kernelHeap;
// tracks allocated memory (realloc)
map<void*, unsigned long, less<void*>, KernelAllocator<void*>> memorySizeMap;

void *KOS_Malloc(unsigned long size) {
  void* memory = kernelHeap.malloc(size);
  if (memory) memorySizeMap[memory] = size;
  return memory;
}

void KOS_Free(void *addr) {
  BSD_KASSERT0(memorySizeMap.count(addr) == 1);   // check memory is in memorySizeMap
  BSD_KASSERT0(memorySizeMap.erase(addr) == 1);   // check it if correctly deleted
  kernelHeap.free(addr);
}

// Get memory size of allocated memory block (for realloc)
unsigned long KOS_GetMemorySize(void *addr) {
  if (memorySizeMap.count(addr)) {
    return memorySizeMap[addr];
  }
  return 0;
}
