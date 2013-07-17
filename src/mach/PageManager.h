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
#ifndef _PageManager_h_
#define _PageManager_h_ 1

#include "mach/CPU.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "kern/Debug.h"
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

public:
  enum Owner : uint64_t {
    Kernel    = P() | G(),
    User      = P() | US()
  };

  enum PageType : uint64_t {
    Code      = 0,
    RoData    = XD(),
    PageTable = RW(),
    Data      = RW() | XD()
  };

private:
  static const uint64_t pageMask = P() | G() | US() | RW() | XD();

  // use second-last entry in PML4 for recursive page directories
  // kernel code/data needs to be at that last 2G for mcmodel=kernel
  static const mword pml4entry = (1 << pagetablebits) - 2;

  // recursively compute page table prefix at level N
  // specialization for <1> below (must be outside of class scope)
  template<unsigned int N> static mword ptp() {
    static_assert( N > 1 && N <= pagelevels, "page level template violation" );
    return (pml4entry << pagesizebits<1+pagelevels-N>()) | ptp<N-1>();
  }

  // ptp<1>() - ptpend() is the VM region occupied by page table
  static constexpr mword ptpend() {
    return VAddr((pml4entry + 1) << pagesizebits<pagelevels>());
  }

  // extract significant bits for level N from virtual address
  template <unsigned int N> static constexpr mword addrprefix( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    return (vma & maskbits(pagesizebits<N>(),pagebits)) >> pagesizebits<N>();
  }

  // add offset to page table prefix to obtain page table entry
  template <unsigned int N> static constexpr PageEntry* getEntry( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    return ((PageEntry*)VAddr(ptp<N>()).ptr()) + addrprefix<N>(vma);
  }

  // compute begining of page table from page entry
  template <unsigned int N> static constexpr PageEntry* getTable( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    return (PageEntry*)align_down(mword(getEntry<N>(vma)),pagetablesize());
  }

  static void configure() {
    MSR::enableNX();                            // enable NX paging bit
    CPU::writeCR4(CPU::readCR4() | CPU::PGE()); // enable  G paging bit
  }

  static laddr bootstrap( FrameManager&fm, mword kernelBase, mword kernelBoot,
    mword kernelCode, mword kernelData, mword kernelEnd, mword pageSize ) {

    // use physical addresses
    kernelBoot -= kernelBase;
    kernelCode -= kernelBase;
    kernelData -= kernelBase;
    kernelEnd  -= kernelBase;

    // must allocate frames that are currently identity-mapped -> below boot code
    PageEntry* pml4 = (PageEntry*)fm.alloc<true>(pagetablesize(), kernelBoot);
    DBG::outln(DBG::Paging, "FM/pml4: ", fm );
    KASSERT1(laddr(pml4) != topaddr, pml4);
    PageEntry* pdpt = (PageEntry*)fm.alloc<true>(pagetablesize(), kernelBoot);
    DBG::outln(DBG::Paging, "FM/pdpt: ", fm );
    KASSERT1(laddr(pdpt) != topaddr, pdpt);
    PageEntry* pd   = (PageEntry*)fm.alloc<true>(pagetablesize(), kernelBoot);
    DBG::outln(DBG::Paging, "FM/pd: ", fm );
    KASSERT1(laddr(pd) != topaddr, pd);
    memset(pml4, 0, pagetablesize());
    memset(pdpt, 0, pagetablesize());
    memset(pd,   0, pagetablesize());

    pml4[pml4entry].c = mword(pml4) | Kernel | PageTable; // recursive directory
    mword idx4 = VAddr(kernelBase).segval<4>();
    pml4[idx4].c = mword(pdpt) | Kernel | PageTable;
//    pml4[0].c = mword(pdpt) | Kernel | PageTable;         // identity-mapping

    mword idx3 = VAddr(kernelBase).segval<3>();
    pdpt[idx3].c = mword(pd) | Kernel | PageTable;
//    pdpt[0].c = mword(pd) | Kernel | PageTable;           // identity-mapping

    vaddr lma = 0;
    size_t i = 0;
    for ( ; lma < kernelCode; i += 1, lma += pageSize) {
      pd[i].c = lma | Kernel | PageTable | PS();
    }
    for ( ; lma < kernelData; i += 1, lma += pageSize) {
      pd[i].c = lma | Kernel | Code | PS();
    }
    for ( ; lma < kernelEnd;  i += 1, lma += pageSize) {
      pd[i].c = lma | Kernel | Data | PS();
    }

    return laddr(pml4);
  }

  template<unsigned int N>
  static inline mword vtolInternal( mword vma ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    PageEntry* pe = getEntry<N>(vma);
    KASSERT1(pe->P, FmtHex(vma));
    if (pe->PS) return (pe->ADDR << pageoffsetbits) + offset<N>(vma);
    else return vtolInternal<N-1>(vma);
  }

  template <unsigned int N>
  static void relabel( vaddr vma, Owner owner, PageType type ) {
    static_assert( N > 0 && N <= pagelevels, "page level template violation" );
    PageEntry* pe = getEntry<N>(vma);
    KASSERT1( pe->P, FmtHex(vma) );
    pe->c = (pe->c & ~pageMask) | owner | type;
    relabel<N+1>(vma, owner, type);
  }

protected:
  static void installAddressSpace(laddr pml4) {
    CPU::writeCR3(pml4);
  }

  // specialization for <pagelevels> below (must be outside of class scope)
  template <unsigned int N>
  static inline void maprecursive( mword vma, Owner owner ) {
    static_assert( N >= 1 && N < pagelevels, "page level template violation" );
    maprecursive<N+1>(vma,owner);
    PageEntry* pe = getEntry<N+1>(vma);
    KASSERT1(N+1 == pagelevels || !pe->PS, FmtHex(vma));
    DBG::out(DBG::Paging, ' ', pe);
    if unlikely(!pe->P) {
      DBG::out(DBG::Paging, 'A');
      laddr lma = Processor::getFrameManager()->alloc(pagetablesize());
      pe->c = owner | PageTable | lma;
      memset( getTable<N>(vma), 0, pagetablesize() );	// TODO: use alloczero later
    }
  }

  template <unsigned int N>
  static inline void map( mword vma, mword lma, Owner owner, PageType type ) {
    static_assert( N >= 1 && N < pagelevels, "page level template violation" );
    KASSERT1( aligned(vma, pagesize<N>()), FmtHex(vma) );
    KASSERT1( (lma & ~ADDR()) == 0, FmtHex(lma) );
    DBG::outln(DBG::Paging, "mapping: ", FmtHex(vma), '/', FmtHex(pagesize<N>()), " -> ", FmtHex(lma));
    maprecursive<N>(vma,owner);
    PageEntry* pe = getEntry<N>(vma);
    DBG::outln(DBG::Paging, ' ', pe);
    KASSERT1( !pe->P, FmtHex(vma) );
    pe->c = owner | type | lma;
    if (N > 1) pe->c |= PS();
  }


  // specialization for <pagelevels-1> below (must be outside of class scope)
  template <unsigned int N>
  static inline void unmaprecursive( mword vma, size_t levels ) {
    static_assert( N >= 1 && N < pagelevels-1, "page level template violation" );
    laddr lma = unmap<N+1>(vma, levels);
    Processor::getFrameManager()->release(lma, pagetablesize());
  }

  template <unsigned int N>
  static inline mword unmap( mword vma, size_t levels = 0 ) {
    static_assert( N >= 1 && N <= pagelevels, "page level template violation" );
    PageEntry* pe = getEntry<N>(vma);
    DBG::outln(DBG::Paging, "unmapping ", FmtHex(vma), '/', FmtHex(pagesize<N>()), ": ", pe);
    KASSERT1( pe->P, FmtHex(vma) );
    pe->P = 0;
    mword ret = pe->ADDR << pageoffsetbits; // retrieve LMA, before recursion
    if (levels > 0) unmaprecursive<N>(vma, levels - 1);
    else CPU::invTLB(vma);
    return ret;
  }

public:
  PageManager() {}
  PageManager(const PageManager&) = delete;            // no copy
  PageManager& operator=(const PageManager&) = delete; // no assignment

  static inline mword vtol( mword vma ) {
    return vtolInternal<pagelevels>(vma);
  }

};

template<> inline mword PageManager::ptp<1>() {
  return VAddr(pml4entry << pagesizebits<pagelevels>());
}

template<> inline void PageManager::maprecursive<pagelevels>( mword, PageManager::Owner ) {}

template<> inline void PageManager::unmaprecursive<pagelevels-1>( mword, size_t ) {}

template<> inline void PageManager::relabel<pagelevels+1>( vaddr, PageManager::Owner, PageManager::PageType ) {}

template<> inline mword PageManager::vtolInternal<1>( mword vma ) {
  PageEntry* pe = getEntry<1>(vma);
  KASSERT1(pe->P, FmtHex(vma));
  return ADDR(pe->c) + offset<1>(vma);
}

#endif /* _PageManager_h_ */
