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
#include "mach/Memory.h"
#include "mach/PageManager.h"
#include "kern/FrameManager.h"
#include "ipc/SpinLock.h"

#include <set>

// TODO: no support for memory overcommit and traditional [s]brk-based heap
// overcommit: compare available with maximum vm region - keep track of type
// heap: ensure that variable mappings don't gratuitously encroach on heap
class AddressSpace : public PageManager {
public:
  using Owner = PageManager::Owner;
  using Type  = PageManager::PageType;

private:
  friend ostream& operator<<(ostream&, const AddressSpace&);

  SpinLock lock;
  laddr pagetable;  // page table address (physical address)
  Owner owner;

  using BuddySet = set<vaddr,less<vaddr>,KernelAllocator<vaddr>>;
  BuddyMap<pagesizebits<1>(),pagebits,BuddySet> unusedVirtAddrRange;

  AddressSpace(const AddressSpace&) = delete;                  // no copy
  const AddressSpace& operator=(const AddressSpace&) = delete; // no assignment

  template<size_t N, bool alloc, bool virt>
  vaddr mapPagesInternal( size_t size, Type type, vaddr lma, vaddr vma ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT1( aligned(vma, pagesize<N>()), vma );
    KASSERT1( aligned(size, pagesize<N>()), size );
    ScopedLock<> lo(lock);
    if (virt) {
      bool check = unusedVirtAddrRange.remove(vma, size);
      KASSERT1(check, (ptr_t)vma);
    } else {
      vma = unusedVirtAddrRange.retrieve(size);
      KASSERT1(vma != topaddr, size);
    }
    vaddr ret = vma;
    for (; size > 0; size -= pagesize<N>()) {
      if (alloc) lma = Processor::getFrameManager()->alloc(pagesize<N>());
      KASSERT1(lma != topaddr, size);
      PageManager::map<N>(vma, lma, owner, type, *Processor::getFrameManager());
      DBG::outln(DBG::VM, "AS/map ", FmtHex(lma), " -> ", FmtHex(vma), " flags: ", PageManager::FmtFlags(owner|type));
      vma += pagesize<N>();
      if (!alloc) lma += pagesize<N>();
    }
    return ret;
  }

  template<size_t N, bool alloc>
  bool unmapPagesInternal( vaddr vma, size_t size ) {
    static_assert( N < pagelevels, "page level template violation" );
    KASSERT1( aligned(vma, pagesize<N>()), vma );
    KASSERT1( aligned(size, pagesize<N>()), size );
    ScopedLock<> lo(lock);
    for (; size > 0; size -= pagesize<N>()) {
      size_t idx = unusedVirtAddrRange.insert2(vma, pagesizebits<N>());
      KASSERTN((idx >= pagesizebits<N>() && idx < pagebits), FmtHex(vma), '/', idx)
      laddr lma = PageManager::unmap<N>(vma, *Processor::getFrameManager(), (idx - pagesizebits<N>()) / pagetablebits);
      if (alloc) Processor::getFrameManager()->release(lma, pagesize<N>());
      DBG::outln(DBG::VM, "AS/unmap ", FmtHex(lma), " -> ", FmtHex(vma));
      vma += pagesize<N>();
    }
    return true;
  }
      
public:
  AddressSpace(Owner o) : pagetable(topaddr), owner(o) {}
  ~AddressSpace() {
    if (pagetable != topaddr) {
      // TODO: cleanup!
    }
  }

  void setMemoryRange(vaddr start, size_t length) {
    unusedVirtAddrRange.insert(start, length);
  }

  void setPagetable(laddr pt) { pagetable = pt; }
  laddr getPagetable() { return pagetable; }
  void activate() {
    KASSERT0(pagetable != topaddr);
    installAddressSpace(pagetable);
  }

  void clonePagetable(AddressSpace& as) {
    pagetable = Processor::getFrameManager()->alloc(pagetablesize());
    vaddr vpt = mapPages<1>(pagetable, pagetablesize(), PageTable);
    vaddr vptorig = mapPages<1>(as.pagetable, pagetablesize(), PageTable);
    PageManager::clone(pagetable, vpt, vptorig);
    unmapPages<1>(vptorig, pagetablesize());
    unmapPages<1>(vpt, pagetablesize());
  }

  template<size_t N> // allocate memory and map to any virtual address
  vaddr allocPages( size_t size, Type t ) { 
    return mapPagesInternal<N,true,false>(size, t, 0, 0);
  }

  template<size_t N> // allocate memory and map to specific virtual address
  vaddr allocPages( vaddr vma, size_t size, Type t ) {
    return mapPagesInternal<N,true,true>(size, t, 0, vma);
  }

  template<size_t N> // map memory to any virtual address
  vaddr mapPages( vaddr lma, size_t size, Type t ) {
    return mapPagesInternal<N,false,false>(size, t, lma, 0);
  }

  template<size_t N> // map memory to specific virtual address
  vaddr mapPages( vaddr lma, vaddr vma, size_t size, Type t ) {
    return mapPagesInternal<N,false,true>(size, t, lma, vma);
  }

  template<size_t N> // free allocated memory
  bool releasePages( vaddr vma, size_t size ) {
    return unmapPagesInternal<N,true>(vma, size);
  }

  template<size_t N> // unmap memory
  bool unmapPages( vaddr vma, size_t size ) {
    return unmapPagesInternal<N,false>(vma, size);
  }

};

#endif /* _AddressSpace_h_ */
