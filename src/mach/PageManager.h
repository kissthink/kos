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
#ifndef _PageManager_h_
#define _PageManager_h_ 1

#include "util/Log.h"
#include "util/Debug.h"
#include "mach/CPU.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "kern/FrameManager.h"

#include <cstring>

/* page map access with recursive PML4
target page vaddr: ABCD|0[000]
PML4 self          XXXX|X[000]
L4 entry           XXXX|A[000]
L3 entry           XXXA|B[000]
L2 entry           XXAB|C[000]
L1 entry           XABC|D[000]
with X = per-level bit pattern (position of recursive entry in PML4). */

class PageManager {
  friend class Machine;
  friend class AddressSpace;

  static const BitSeg<uint64_t, 0, 1> P;
  static const BitSeg<uint64_t, 1, 1> RW;
  static const BitSeg<uint64_t, 2, 1> US;
  static const BitSeg<uint64_t, 3, 1> PWT;
  static const BitSeg<uint64_t, 4, 1> PCD;
  static const BitSeg<uint64_t, 5, 1> A;
  static const BitSeg<uint64_t, 6, 1> D;
  static const BitSeg<uint64_t, 7, 1> PS;
  static const BitSeg<uint64_t, 8, 1> G;
  static const BitSeg<uint64_t,12,40> ADDR;
  static const BitSeg<uint64_t,63, 1> XD;

  // use second-last entry in PML4 for recursive page directories
  // kernel code/data needs to be at last 2G for mcmodel=kernel
  static const mword pml4entry = (1 << pagetablebits) - 2;

  // recursively compute page table prefix at level N
  // specialization for <1> below (must be outside of class scope)
  template<unsigned int N> static constexpr mword ptp() {
    static_assert( N > 1 && N <= pagelevels, "page level template violation" );
    return (pml4entry << pagesizebits<1+pagelevels-N>()) | ptp<N-1>();
  }

  // extract significant bits for level N from virtual address
  template <unsigned int N> static constexpr mword addrprefix(mword vma) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    return maskbits(vma,pagesizebits<N>(),pagebits) >> pagesizebits<N>();
  }

  // add offset to page table prefix to obtain page table entry
  template <unsigned int N> static constexpr PageEntry* getEntry( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    return ((PageEntry*)VAddr(ptp<N>()).ptr()) + addrprefix<N>(vma);
  }

  // compute begining of page table from page entry
  template <unsigned int N> static constexpr PageEntry* getTable( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    return (PageEntry*)align_down(mword(getEntry<N>(vma)),pagesize<1>());
  }

  enum PageOwner : uint64_t {
    Kernel = P() | G(),
    User   = P() | US()
  };

  enum PageType : uint64_t {
    Code   = 0,
    DataRO = XD(),
    Data   = RW() | XD()
  };

  static inline laddr bootstrap( FrameManager&fm, mword kernelBase,
    mword kernelCodePC, mword kernelCodePS,
    mword kernelDataPC, mword kernelDataPS, mword mappedLimit ) {

    static_assert( sizeof(PageEntry) == sizeof(mword), "PageEntry != mword" );

    // need to allocate frames that are identity mapped -> use mappedLimit
    PageEntry* pml4 = (PageEntry*)fm.allocPow2(pageoffsetbits, mappedLimit);
    DBG::outln(DBG::Paging, "FM/pml4: ", fm );
    KASSERT(laddr(pml4) != illaddr, pml4);
    PageEntry* pdpt = (PageEntry*)fm.allocPow2(pageoffsetbits, mappedLimit);
    DBG::outln(DBG::Paging, "FM/pdpt: ", fm );
    KASSERT(laddr(pdpt) != illaddr, pdpt);
    PageEntry* pd   = (PageEntry*)fm.allocPow2(pageoffsetbits, mappedLimit);
    DBG::outln(DBG::Paging, "FM/pd: ", fm );
    KASSERT(laddr(pd) != illaddr, pd);
    memset(pml4, 0, sizeof(PageEntry) << pagetablebits );
    memset(pdpt, 0, sizeof(PageEntry) << pagetablebits );
    memset(pd,   0, sizeof(PageEntry) << pagetablebits );

    // create PML4 entries
    mword idx4 = VAddr(kernelBase).val<4>();
    pml4[idx4].c = mword(pdpt) | Kernel;
//    pml4[0].c = mword(pdpt) | Kernel;      // identity-mapping

    // create PDPT entries
    mword idx3 = VAddr(kernelBase).val<3>();
    pdpt[idx3].c = mword(pd) | Kernel;
//    pdpt[0].c = mword(pd) | Kernel;        // identity-mapping

    // create PD entries
    mword x = 0;
    for ( ; x < kernelCodePC; x += 1) {
      pd[x].c = x * kernelCodePS | Kernel | Code | PS();
    }
    for ( ; x < kernelDataPC; x += 1) {
      pd[x].c = x * kernelDataPS | Kernel | RW() | PS();
    }

    // enable recursive page directory
    pml4[pml4entry].c = mword(pml4) | Kernel;
    return laddr(pml4);
  }

  static void installAddressSpace(laddr pml4) {
    CPU::writeCR3(pml4);
  }

  // specialization for <1> and <pagelevels> below (must be outside of class scope)
  template <unsigned int N>
  static inline void mappage( PageEntry* pe ) {
    static_assert( N > 1 && N < pagelevels, "illegal page size direct mapping" );
    pe->c |= PS();
  }

  // specialization for <pagelevels> below (must be outside of class scope)
  template <unsigned int N>
  static inline void maprecursive( mword vma, PageOwner o ) {
    static_assert( N >= 1 && N < pagelevels, "page level template violation" );
    maptable<N+1>( vma, o );
  }

  template <unsigned int N>
  static inline void maptable( mword vma, PageOwner o ) {
    static_assert( N > 1 && N <= pagelevels, "page level template violation" );
    maprecursive<N>(vma, o);
    PageEntry* pe = getEntry<N>(vma);
    KASSERT(N == pagelevels || !pe->PS, (ptr_t)vma);
    DBG::out(DBG::Paging, ' ', pe);
    if unlikely(!pe->P) {
      DBG::out(DBG::Paging, 'A');
      pe->c = o | Processor::getFrameManager()->allocPow2(pageoffsetbits);
      memset( getTable<N-1>(vma), 0, pagesize<1>() );	// TODO: use alloczero later
    }
  }

  template <unsigned int N>
  static inline void map( mword vma, mword lma, PageOwner o, PageType t, bool strict ) {
    static_assert( N >= 1 && N < pagelevels, "page level template violation" );
    KASSERT( bitaligned(vma, pagesizebits<N>()), (ptr_t)vma );
    KASSERT( (lma & ~ADDR()) == 0, (ptr_t)lma );
    DBG::outln(DBG::Paging, "mapping: ", (ptr_t)vma, '/', (ptr_t)pagesize<N>(), " -> ", (ptr_t)lma);
    maprecursive<N>(vma, o);
    PageEntry* pe = getEntry<N>(vma);
    DBG::outln(DBG::Paging, ' ', pe);
    KASSERT( !strict || !pe->P, (ptr_t)vma );
    pe->c = o | t | lma;
    mappage<N>(pe);
  }  

  // TODO: only unmaps lowest level
  template <unsigned int N>
  static inline mword unmap( mword vma, bool strict ) {
    static_assert( N >= 1 && N < pagelevels, "page level template violation" );
    PageEntry* pe = getEntry<N>(vma);
    DBG::outln(DBG::Paging, "unmapping ", (ptr_t)vma, '/', (ptr_t)pagesize<N>(), ": ", pe);
    KASSERT(!strict || pe->P, (ptr_t)vma);
    pe->P = 0;
    return pe->ADDR;
  }

public:
  PageManager() = delete;                              // no creation
  PageManager(const PageManager&) = delete;            // no copy
  PageManager& operator=(const PageManager&) = delete; // no assignment

  template<unsigned int N>
  static inline mword vtol( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    PageEntry* pe = getEntry<N>(vma);
    KASSERT(pe->P, (ptr_t)vma);
    if (pe->PS) return ADDR(pe->c) + offset<N>(vma);
    else return vtol<N-1>(vma);
  }

};

template<> inline constexpr mword PageManager::ptp<1>() {
  return pml4entry << pagesizebits<4>();
}

template<> inline void PageManager::maprecursive<pagelevels>( mword vma, PageOwner o ) {}

template<> inline void PageManager::mappage<1>( PageEntry* pe ) {}

template<> inline void PageManager::mappage<pagelevels>( PageEntry* pe ) {}

template<> inline mword PageManager::vtol<1>( mword vma ) {
  PageEntry* pe = getEntry<1>(vma);
  KASSERT(pe->P, (ptr_t)vma);
  return ADDR(pe->c) + offset<1>(vma);
}

#endif /* _PageManager_h_ */
