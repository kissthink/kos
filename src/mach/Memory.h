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
#ifndef _Memory_h_
#define _Memory_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

/* Typical (old style) PC memory map
      0		  3FF 	Interrupt Vectors (IVT)
    400		  500	BIOS data area(BDA)
    500		9FBFF	free (INT 0x12 with return AX KB blocks, or use E820)
  9FC00		9FFFF	extended BIOS data area (EBDA)
  A0000 	BFFFF	VGA framebuffers
  C0000		C7FFF	video BIOS
  C8000		EFFFF	nothing
  F0000		FFFFF	motherboard BIOS
 100000 .....		mostly free (1MB and above) */

static const size_t pageoffsetbits = 12;
static const size_t pagetablebits = 9;
static const size_t pagelevels = 4;
static const size_t framebits = pageoffsetbits + 40;
static const size_t pagebits = pageoffsetbits + pagetablebits * pagelevels;

template<unsigned int N>
static constexpr size_t pagesizebits() {
  static_assert( N <= pagelevels, "page level template violation" );
  return pageoffsetbits + (N-1) * pagetablebits;
}

template<unsigned int N>
static constexpr size_t pagesize() {
  static_assert( N <= pagelevels, "page level template violation" );
  return pow2(pagesizebits<N>());
}

template<unsigned int N>
static constexpr size_t offset(mword vma) {
  static_assert( N <= pagelevels, "page level template violation" );
  return (pow2(pagesizebits<N>()) - 1) & vma;
}

static const vaddr kernelBase = KERNBASE;
static const vaddr kernelLow  = kernelBase & ~maskbits(pagebits-1);
static const vaddr deviceAddr = align_up(kernelBase + BOOTMEM, pagesize<3>()) - pagesize<2>();
static const vaddr videoAddr  = deviceAddr + 0 * pagesize<1>();
static const vaddr lapicAddr  = deviceAddr + 1 * pagesize<1>();
static const vaddr ioapicAddr = deviceAddr + 2 * pagesize<1>();

class VAddr {
  static const mword canonical = mwordlimit - maskbits(pagebits);
  mword addr;
public:
  VAddr() = default;
  VAddr(const VAddr&) = default;
  VAddr& operator=(const VAddr&) = default;
  constexpr VAddr(mword a) : addr((a & pow2(pagebits-1)) ? (canonical | a) : a) {}
  
  template<unsigned int N> constexpr mword seg() const {
    return addr & (maskbits(pagetablebits) << pagesizebits<N>());
  }
  template<unsigned int N> constexpr mword val() const {
    return seg<N>() >> pagesizebits<N>();
  }
  constexpr ptr_t ptr() const { return ptr_t(addr); }
  constexpr operator mword() const { return addr; }
};

union PageEntry { // see Intel Vol 3, Section 4.5 "IA-32E Paging"
  uint64_t c;     // compact representation
  struct {        // detailed representation:
		uint64_t P     :  1; // present 
		uint64_t RW    :  1; // read/write
		uint64_t US    :  1; // user/supervisor - 1 -> user/CPL3 can access
		uint64_t PWT   :  1; // page-level write-through
		uint64_t PCD   :  1; // page-level cache disable
		uint64_t A     :  1; // accessed
		uint64_t D     :  1; // dirty
		uint64_t PS    :  1; // PS: 1 -> page, otherwise page table
		uint64_t G     :  1; // global
		uint64_t pad9  :  3; // reserved
		uint64_t ADDR  : 40; // pyhsical address (shift by 12) of next-level entry
		uint64_t pad52 : 11; // reserved
		uint64_t XD    :  1; // execute disable (if EFER.NXE = 1)
	};
	// PAT: memory type (with PWT, PCD)
	template<unsigned int N> inline uint64_t PAT() {
	  static_assert( N > 1 && N < 4, "illegal template parameter" );
	  return ADDR & 1;
  }
} __packed;

template<> inline uint64_t PageEntry::PAT<1>() { return PS; }

struct PageFaultFlags {
  uint64_t flags;
  static const BitSeg<uint64_t, 0, 1> P;		// page not present
  static const BitSeg<uint64_t, 1, 1> WR;		// write not allowed
  static const BitSeg<uint64_t, 2, 1> US;		// user access not allowed
  static const BitSeg<uint64_t, 3, 1> RSVD;	// reserved flag in paging structure
  static const BitSeg<uint64_t, 4, 1> ID;		// instruction fetch
  PageFaultFlags( uint64_t f ) : flags(f) {}
  operator uint64_t() { return flags; }
  operator ptr_t() { return (ptr_t)flags; }
};

#endif /* _Memory_h_ */
