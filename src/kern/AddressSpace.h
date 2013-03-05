/******************************************************************************
    Copyright 2012 Martin Karsten

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

/* AddressSpace manages the available virtual memory region in 'availVM'. 
 * Currently, it only manages the heap, but this will be extended to code,
 * ro, data, etc.  Also, page fault handling could be redirected here to
 * implement memory overcommit per address space.  */

class AddressSpace : public PageManager {
  friend ostream& operator<<(ostream&, const AddressSpace&);

  SpinLock lock;
  laddr pml4;
  using BuddySet = std::set<vaddr,std::less<vaddr>,KernelAllocator<vaddr>>;
  BuddyMap<pagesizebits<1>(),pagebits,BuddySet> availVM;

public:
  void init( laddr pml4, vaddr start, vaddr end ) {
    this->pml4 = pml4;
    availVM.insert(start, end - start);
  }

  void activate() {
    PageManager::installAddressSpace(pml4);
  }

  template<size_t N, bool alloc = false, bool virt = false>
  vaddr mapPages( size_t size, vaddr lma = 0, vaddr vma = 0 ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    LockObject<SpinLock> lo(lock);
    if (virt) {
      bool check = availVM.remove(vma, size);
      KASSERT(check, (ptr_t)vma);
    } else {
      vma = availVM.retrieve(size);
      KASSERT(vma != illaddr, size);
    }
    vaddr ret = vma;
    for (; size > 0; size -= pagesize<N>()) {
      if (alloc) lma = Processor::getFrameManager()->alloc(pagesize<N>());
      PageManager::map<N>(vma, lma, PageManager::Kernel, PageManager::Data);
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
    LockObject<SpinLock> lo(lock);
    for (; size > 0; size -= pagesize<N>()) {
      size_t idx = availVM.insert2(vma, pagesizebits<N>());
      KASSERT( (idx >= pagesizebits<N>() && idx < pagebits), (ptr_t)vma)
      laddr lma = PageManager::unmap<N>(vma, (idx - pagesizebits<N>()) / pagetablebits);
      if (alloc) Processor::getFrameManager()->release(lma, pagesize<N>());
      vma += pagesize<N>();
    }
    return true;
  }
};

extern inline ostream& operator<<(ostream& os, const AddressSpace& as) {
  os << as.availVM;
  return os;
}

#endif /* _AddressSpace_h_ */
