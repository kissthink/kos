/******************************************************************************
    Copyright 2012-2013 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef _AddressSpace_h_
#define _AddressSpace_h_ 1

#include "util/BuddyMap.h"
#include "util/SpinLock.h"
#include "mach/Memory.h"
#include "mach/PageManager.h"
#include "mach/Processor.h"
#include "kern/FrameManager.h"

#include <set>

class AddressSpace : public PageManager {
  friend std::ostream& operator<<(std::ostream&, const AddressSpace&);

  SpinLock lock;
  laddr pagetable;  // page table address (physical)
  vaddr vmStart;    // start of virtual memory (text,ro,data,bss)
  vaddr heapStart;  // start of heap
  vaddr heapEnd;    // end of heap
  vaddr allocEnd;   // end of allocated range (page-aligned)
  vaddr mapStart;   // start of map segment
  vaddr mapEnd;     // end of map segment
  vaddr vmEnd;      // end of virtual memory address range

  PageManager::PageOwner owner;

  using BuddySet = std::set<vaddr,std::less<vaddr>,KernelAllocator<vaddr>>;
  BuddyMap<pagesizebits<1>(),pagebits,BuddySet> availableMemory;

  AddressSpace(const AddressSpace&) = delete;                  // no copy
  const AddressSpace& operator=(const AddressSpace&) = delete; // no assignment

public:
  AddressSpace() : pagetable(topaddr) {}
  ~AddressSpace() {
    if (pagetable != topaddr) {
      // TODO: cleanup!
    }
  }

  void setMemoryRange(vaddr vs, vaddr hs, vaddr ae, vaddr ms, vaddr me, vaddr ve, PageManager::PageOwner o) {
    vmStart = vs;
    heapStart = heapEnd = hs;
    allocEnd = ae;
    mapStart = ms;
    mapEnd = me;
    vmEnd = ve;
    owner = o;
    availableMemory.insert(ms, me-ms);
  }

  void setPagetable(laddr pt) { pagetable = pt; }

  void clonePagetable(AddressSpace& as) {
    pagetable = Processor::getFrameManager()->alloc(pagetablesize());
    vaddr vpt = as.mapPages<1>(pagetablesize(), pagetable);
    vaddr vptorig = as.mapPages<1>(pagetablesize(), as.pagetable);
    memcpy(ptr_t(vpt), ptr_t(vptorig), pagetablesize());
    as.unmapPages<1>(vptorig, pagetablesize());
    as.unmapPages<1>(vpt, pagetablesize());
  }

  void activate() { PageManager::installAddressSpace(pagetable); }

  template<size_t N, bool alloc = false, bool virt = false>
  vaddr mapPages( size_t size, vaddr lma = 0, vaddr vma = 0 ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    ScopedLock<> lo(lock);
    if (virt) {
      bool check = availableMemory.remove(vma, size);
      KASSERT(check, (ptr_t)vma);
    } else {
      vma = availableMemory.retrieve(size);
      KASSERT(vma != topaddr, size);
    }
    vaddr ret = vma;
    for (; size > 0; size -= pagesize<N>()) {
      if (alloc) lma = Processor::getFrameManager()->alloc(pagesize<N>());
      KASSERT(lma != topaddr, size);
      PageManager::map<N>(vma, lma, owner, PageManager::Data);
      vma += pagesize<N>();
      if (!alloc) lma += pagesize<N>();
    }
    return ret;
  }

  template<size_t N, bool alloc = false>
  bool unmapPages( vaddr vma, size_t size ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    ScopedLock<> lo(lock);
    for (; size > 0; size -= pagesize<N>()) {
      size_t idx = availableMemory.insert2(vma, pagesizebits<N>());
      KASSERT((idx >= pagesizebits<N>() && idx < pagebits), (ptr_t)vma)
      laddr lma = PageManager::unmap<N>(vma, (idx - pagesizebits<N>()) / pagetablebits);
      if (alloc) Processor::getFrameManager()->release(lma, pagesize<N>());
      vma += pagesize<N>();
    }
    return true;
  }
      
};

#endif /* _AddressSpace_h_ */
