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
#ifndef _Descriptors_h_
#define _Descriptors_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

// used for GDT
struct SegmentDescriptor { // see Intel Vol. 3, Section 3.4.5 "Segment Descriptors"
  uint64_t Limit00  : 16; // ignored in 64-bit mode
  uint64_t Base00   : 16; // ignored in 64-bit mode
  uint64_t Base16   :  8; // ignored in 64-bit mode
  uint64_t A        :  1; // accessed / ignored in 64-bit mode
  uint64_t RW       :  1; // readable (code), writable (data) -> not ignored by qemu
  uint64_t E        :  1; // expand-down / ignored in 64-bit mode
  uint64_t C        :  1; // code
  uint64_t S        :  1; // available for system software / must be 1 in 64-bit mode
  uint64_t DPL      :  2; // privilege level
  uint64_t P        :  1; // present
  uint64_t Limit16  :  4; // ignored in 64-bit mode
  uint64_t AVL      :  1; // flag can be used by kernel / ignored in 64-bit mode
  uint64_t L        :  1; // 64-bit mode segment (vs. compatibility mode)
  uint64_t DB       :  1; // default operation size / ignored for 64-bit segment
  uint64_t G        :  1; // granularity/scaling / ignored in 64-bit mode
  uint64_t Base24   :  8; // ignored in 64-bit mode
} __packed;

// special case of system segment descriptor, used for IDT
struct GateDescriptor { // see Intel Vol 3, Section 6.14.1 "64-Bit Mode IDT"
  uint64_t Offset00       :16;
  uint64_t SegmentSelector:16;
  uint64_t IST            : 3; // interrupt stack table
  uint64_t Reserved1      : 5;
  uint64_t Type           : 4; // type, see Intel Vol. 3, Section 3.5 "System Descriptor Types"
  uint64_t Reserved2      : 1;
  uint64_t DPL            : 2; // privilege level
  uint64_t P              : 1; // present
  uint64_t Offset16       :16;
  uint64_t Offset32       :32;
  uint64_t Reserved3      :32;
} __packed;

struct SelectorErrorFlags { // see AMD Vol. 2, Section 8.4.1 "Selector-Error Code"
  uint64_t flags;
  static const BitSeg<uint64_t, 0, 1> EXT;   // external exception source?
  static const BitSeg<uint64_t, 1, 1> IDT;   // descriptor in IDT? (vs GDT/LDT)
  static const BitSeg<uint64_t, 2, 1> TI;    // descriptor in LDT? (vs GDT)
  static const BitSeg<uint64_t, 3,13> Index; // selector index
  SelectorErrorFlags( uint64_t f ) : flags(f) {}
  operator uint64_t() { return flags; }
  operator ptr_t() { return (ptr_t)flags; }
};

#endif /* _Descriptors_h_ */
