#include "impl/CdiMemoryManager.h"

// KOS
#include "mach/Processor.h"
#include "kern/Kernel.h"
#include "util/Output.h"

static const size_t MEM_2MB  = 0x200000;
static const size_t MEM_16MB = 0x1000000;
static const size_t MEM_4GB  = 0x100000000;

cdi_mem_area* CdiMemoryManager::alloc(size_t size, cdi_mem_flags_t flags) {
  mword alignSize = 1 << (flags & CDI_MEM_ALIGN_MASK);
  DBG::outln(DBG::VM, "cdi_mem_alloc() align size: ", FmtHex(alignSize));
  if ( size < pagesize<1>() ) size = pagesize<1>(); // allocate in pages
  if (!aligned(size, pagesize<1>())) {
    size = align_up(size, pagesize<1>());
  }
  DBG::outln(DBG::VM, "cdi_mem_alloc() aligned size: ", FmtHex(size));
  if ( flags & CDI_MEM_PHYS_CONTIGUOUS ) {
    bool under16MB = flags & CDI_MEM_DMA_16M;
    bool under4G = flags & CDI_MEM_DMA_4G;
    // use FrameManager to allocate physically contiguous memory
    laddr phyAddr = 0;
    if ( under16MB ) {
      phyAddr = Processor::getFrameManager()->alloc<true>( size, MEM_16MB );   // 16mb
    } else if ( under4G ) {
      phyAddr = Processor::getFrameManager()->alloc<true>( size, MEM_4GB ); // 4G
    } else {
      phyAddr = Processor::getFrameManager()->alloc<false>( size );
    }
    if (phyAddr == 0) {
      DBG::outln(DBG::VM, "cdi_mem_alloc() failed allocating physical memory");
      return nullptr;
    }
    KASSERT1( aligned(phyAddr, alignSize), phyAddr);

    vaddr vAddr = 0;
    if ( size >= 0x200000 ) { // use 2MB paging
      vAddr = kernelSpace.mapPages<2>( phyAddr, size, AddressSpace::Data );
    } else {
      vAddr = kernelSpace.mapPages<1>( phyAddr, size, AddressSpace::Data );
    }
    if (vAddr == 0) {
      DBG::outln(DBG::VM, "cdi_mem_alloc() failed mapping physical memory: ", FmtHex(phyAddr));
      return nullptr;
    }

    cdi_mem_area* area = new cdi_mem_area;
    area->size = size;
    area->vaddr = (ptr_t)vAddr;
    area->paddr.num = 1;
    area->paddr.items = new cdi_mem_sg_item;
    area->paddr.items->start = phyAddr;
    area->paddr.items->size = size;
    return area;
  } else {
    vaddr vAddr = 0;
    if ( size >= 0x200000 ) { // use 2MB paging
      vAddr = kernelSpace.allocPages<2>( size, AddressSpace::Data );
    } else {
      vAddr = kernelSpace.allocPages<1>( size, AddressSpace::Data );
    }
    if (vAddr == 0) {
      DBG::outln(DBG::VM, "cdi_mem_alloc() failed allocating ", FmtHex(size), " bytes");
      return nullptr;
    }

    cdi_mem_area* area = new cdi_mem_area;
    area->size = size;
    area->vaddr = (ptr_t)vAddr;
    // find physical address for each page
    mword pageSize = ( size >= 0x200000 ? pagesize<2>() : pagesize<1>() );
    mword curVAddr = vAddr;
    mword numSgItems = size/pageSize;
    if ( size % pageSize ) numSgItems += 1;
    cdi_mem_sg_item* sgItems = new cdi_mem_sg_item[ numSgItems ];
    area->paddr.items = sgItems;
    for (mword s = size; s > 0; s -= pageSize) {
      KASSERT1( area->paddr.num < numSgItems, area->paddr.num );
      mword phyAddr = PageManager::vtol( curVAddr );
      area->paddr.items[ area->paddr.num ].start = phyAddr;
      KASSERT1( aligned( phyAddr, alignSize ), phyAddr );
      area->paddr.items[ area->paddr.num ].size = pageSize;
      curVAddr += pageSize;
      area->paddr.num += 1;
    }
    KASSERT1( numSgItems == area->paddr.num, area->paddr.num );
    return area;
  }
}

cdi_mem_area* CdiMemoryManager::mapPhysical(uintptr_t paddr, size_t size) {
  // I think we can ignore FrameManager failing to reserve physical address
  // for us because these physical addresses (from PCI BAR) may be
  // from device memory
#if 0
  bool reserved = Processor::getFrameManager()->reserve( laddr(paddr), size );
  if (reserved) {
    DBG::outln(DBG::VM, "cdi_mem_map() reserved physical memory: ", FmtHex(paddr), '/', FmtHex(size));
    cdi_mem_area* area = new cdi_mem_area;
    area->size = size;
    area->vaddr = nullptr;
    area->paddr.num = 1;
    area->paddr.items = new cdi_mem_sg_item;
    area->paddr.items->start = paddr;
    area->paddr.items->size = size;
    return area;
  }
#endif
  vaddr mappedAddr = topaddr;
  if ( size >= 0x200000 ) { // use 2MB paging
    mappedAddr = kernelSpace.mapPages<2>( laddr(paddr), size, AddressSpace::Data);
  } else {
    mappedAddr = kernelSpace.mapPages<1>( laddr(paddr), size, AddressSpace::Data);
  }
  if (mappedAddr != topaddr) {
    DBG::outln(DBG::VM, "cdi_mem_map() mapped physical memory: ", FmtHex(paddr), " -> ", FmtHex(mappedAddr));
    cdi_mem_area* area = new cdi_mem_area;
    area->size = size;
    area->vaddr = (ptr_t)mappedAddr;
    area->paddr.num = 1;
    area->paddr.items = new cdi_mem_sg_item;
    area->paddr.items->start = paddr;
    area->paddr.items->size = size;
    return area;
  }
  return nullptr;
}

// Frees a memory area that was previsouly allocated by cdi_mem_alloc or cdi_mem_map
void CdiMemoryManager::free(cdi_mem_area* area) {
  KASSERT0(area);
  if (area->vaddr) {  // unmap from page table
    if (area->size >= MEM_2MB) {
      KASSERT0( kernelSpace.unmapPages<2>( vaddr(area->vaddr), area->size ) );
    } else {
      KASSERT0( kernelSpace.unmapPages<1>( vaddr(area->vaddr), area->size ) );
    }
  }
  for (size_t i = 0; i < area->paddr.num; i++) {
    KASSERT0( Processor::getFrameManager()->release( area->paddr.items[i].start,
                                                     area->paddr.items[i].size ) );
  }
  if (area->paddr.num == 1) {
    delete area->paddr.items;
  } else {
    delete [] area->paddr.items;
  }
  delete area;
}
