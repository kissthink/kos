#include "cdi/dma.h"
#include "cdi/mem.h"
#include "cdi/printf.h"
#include "impl/CdiMemoryManager.h"

// KOS
#include "mach/DMA.h"
#include "util/basics.h"
#include <cstring>

int cdi_dma_open(cdi_dma_handle* handle, uint8_t channel, uint8_t mode,
                 size_t length, ptr_t buffer) {
  handle->channel = channel;
  handle->length = length;
  handle->mode = mode;
  handle->buffer = buffer;

  // allocate memory for DMA
  cdi_mem_flags_t flags = static_cast<cdi_mem_flags_t>(CDI_MEM_PHYS_CONTIGUOUS | CDI_MEM_DMA_16M);
  cdi_mem_area* area = CdiMemoryManager::alloc(length, flags);
  if (!area) return -1;     // failed to get required memory

//  handle->buffer = area->vaddr;
//  memset(handle->buffer, 0, handle->length);
  laddr physAddr = area->paddr.items->start;
  handle->meta.area = area;

  // start DMA
  bool started = DMA::startTransfer(channel, mode, length, physAddr);
  return started ? 0 : -1;
}

int cdi_dma_read(cdi_dma_handle* handle) {
  memcpy(handle->buffer, handle->meta.area->vaddr, handle->length);
  CdiPrintf("reading: ");
  for (unsigned int i = 0; i < handle->length; i++) {
    CdiPrintf("%x", ((char *)handle->buffer)[i]);
  }
  CdiPrintf("\n");
  return 0;
}

int cdi_dma_write(cdi_dma_handle* handle) {
  memcpy(handle->meta.area->vaddr, handle->buffer, handle->length);
  CdiPrintf("writing: ");
  for (unsigned int i = 0; i < handle->length; i++) {
    CdiPrintf("%x", ((char *)handle->buffer)[i]);
  }
  CdiPrintf("\n");
  return 0;
}

int cdi_dma_close(cdi_dma_handle* handle) {
  CdiMemoryManager::free(handle->meta.area);
  return 0;
}