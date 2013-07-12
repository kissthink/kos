#include <cstdint>
#include <cstring>

#include "capi/Memory.h"
#include "kern/KernelVM.h"

extern KernelVM kernelVM;

void *kmalloc(size_t size, gfp_t flags) {
//  if (flags != GFP_KERNEL) return 0;
  return (void *)kernelVM.alloc(size);
}

void *kzalloc(size_t size, gfp_t flags) {
  void *mem = kmalloc(size, flags);
  if (mem != 0) memset(mem, 0, size);
  return mem;
}
