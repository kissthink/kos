#include <cstdint>
#include <cstring>

#include "extern/linux/kos/Memory.h"
#include "kern/KernelHeap.h"

extern KernelHeap kernelHeap;

void *kmalloc(size_t size, gfp_t flags) {
//  if (flags != GFP_KERNEL) return 0;
  return (void *)kernelHeap.alloc(size);
}

void *kzalloc(size_t size, gfp_t flags) {
  void *mem = kmalloc(size, flags);
  if (mem != 0) memset(mem, 0, size);
  return mem;
}
