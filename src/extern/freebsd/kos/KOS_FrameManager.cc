#include "kos/FrameManager.h"

// KOS
#include "kern/FrameManager.h"
#include "mach/platform.h"
#include "mach/Processor.h"

// FrameManager in KOS is a single instance retrievable through Processor class
extern "C" void* KOS_Alloc_Contig(unsigned long size, vaddr low, vaddr high, unsigned long alignment,
                       unsigned long boundary) {
  FrameManager* fm = Processor::getFrameManager();

  // TODO ignore alignment and boundary for now
  laddr addr = fm->alloc<true>(size, low, high);
  if (addr == topaddr) return NULL;      // allocation failed
  return (void *)addr;
}

extern "C" void KOS_Free_Contig(void* addr, unsigned long size) {
  FrameManager* fm = Processor::getFrameManager();
  fm->release((laddr)addr, size);
}
