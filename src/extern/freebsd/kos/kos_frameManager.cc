#include "kos/kos_frameManager.h"
#include "kern/FrameManager.h"
#include "mach/platform.h"
#include "mach/Processor.h"

// FrameManager in KOS is a single instance retrievable through Processor class
void* kos_alloc_contig(unsigned long size, vaddr low, vaddr high, unsigned long alignment,
                       unsigned long boundary) {
  FrameManager* fm = Processor::getFrameManager();

  // TODO ignore alignment and boundary for now
  laddr addr = fm->alloc<true>(size, low, high);
  if (addr == topaddr) return NULL;      // allocation failed
  return (void *)addr;
}

void kos_free_contig(void* addr, unsigned long size) {
  FrameManager* fm = Processor::getFrameManager();
  fm->release((laddr)addr, size);
}
