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

class AddressSpace {
  friend ostream& operator<<(ostream&, const AddressSpace&);

  SpinLock lock;
  laddr pml4;
  vaddr heapStart;
  vaddr heapEnd;
  using BuddySet = std::set<vaddr,std::less<vaddr>,KernelAllocator<vaddr>>;
  BuddyMap<pagesizebits<1>(),pagebits,BuddySet> availVM;

public:
  void init( laddr pml4, vaddr heapStart, vaddr heapEnd ) {
    this->pml4 = pml4;
    this->heapStart = heapStart;
    this->heapEnd = heapEnd;
    availVM.insert(heapStart, heapEnd - heapStart);
  }

  void activate() {
    PageManager::installAddressSpace(pml4);
  }

  template<size_t N>
  vaddr allocPages( size_t size ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(size, pagesize<N>()), size );
    LockObject<SpinLock> lo(lock);
    vaddr vma = availVM.retrieve(size);
    KASSERT(vma != illaddr, size);
    size_t count = (size >> pagesizebits<N>());
    for ( ;count > 0; count -= 1 ) {
      laddr lma = Processor::getFrameManager()->allocPow2(pagesizebits<N>());
      PageManager::map<N>(vma, lma, PageManager::Kernel, PageManager::Data, true);
      vma += pagesize<N>();
    }
    return vma;
  }

  template<size_t N>
  bool allocPages( vaddr vma, size_t size ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    LockObject<SpinLock> lo(lock);
    bool check = availVM.remove(vma, size);
    KASSERT(check, (ptr_t)vma);
    size_t count = (size >> pagesizebits<N>());
    for ( ;count > 0; count -= 1 ) {
      laddr lma = Processor::getFrameManager()->allocPow2(pagesizebits<N>());
      PageManager::map<N>(vma, lma, PageManager::Kernel, PageManager::Data, true);
      vma += pagesize<N>();
    }
    return true;
  }

  template<size_t N>
  bool releasePages( vaddr vma, size_t size ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    LockObject<SpinLock> lo(lock);
    bool check = availVM.insert(vma, size);
    KASSERT(check, (ptr_t)vma)
    size_t count = (size >> pagesizebits<N>());
    for ( ;count > 0; count -= 1 ) {
      laddr lma = PageManager::unmap<N>(vma, true);
      Processor::getFrameManager()->release(lma, pagesize<N>());
      vma += pagesize<N>();
    }
    // TODO: check if higher-level mappings can also be removed
    return true;
  }

  // NOTE: no locking - only used during bootstrap
  template<size_t N>
  void mapPages( vaddr vma, laddr lma, size_t size, bool strict = true ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    if ( vma >= heapStart && vma < heapEnd ) {
      bool check = availVM.remove(vma, size);
      KASSERT(check, (ptr_t)vma);
    }
    size_t count = (size >> pagesizebits<N>());
    for ( ;count > 0; count -= 1 ) {
      PageManager::map<N>(vma, lma, PageManager::Kernel, PageManager::Data, strict);
      vma += pagesize<N>();
      lma += pagesize<N>();
    }
  }

  // NOTE: no locking - only used during bootstrap
  template<size_t N>
  void unmapPages( vaddr vma, size_t size, bool strict = true ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT( aligned(vma, pagesize<N>()), vma );
    KASSERT( aligned(size, pagesize<N>()), size );
    if ( vma >= heapStart && vma < heapEnd ) {
      bool check = availVM.insert(vma, size);
      KASSERT(check, (ptr_t)vma);
    }
    size_t count = (size >> pagesizebits<N>());
    for ( ;count > 0; count -= 1 ) {
      PageManager::unmap<N>(vma, strict);
      vma += pagesize<N>();
    }
    // TODO: check if higher-level mappings can also be removed
  }
};

extern inline ostream& operator<<(ostream& os, const AddressSpace& as) {
  os << "AS: " << as.availVM;
  return os;
}

#endif /* _AddressSpace_h_ */
