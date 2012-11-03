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
#ifndef _FrameManager_h_
#define _FrameManager_h_ 1

#include "util/BuddyMap.h"
#include "util/Debug.h"
#include "kern/KernelHeap.h"

#include <set>

class FrameManager {
  friend class Machine;
  friend class AddressSpace;
  friend class PageManager;
  friend ostream& operator<<(ostream&, const FrameManager&);

  using BuddySet = std::set<vaddr,std::less<vaddr>,KernelAllocator<vaddr>>;
  BuddyMap<pageoffsetbits,framebits,BuddySet> bmap;

  laddr allocPow2( size_t logsize, vaddr limit = mwordlimit ) {
    vaddr addr = bmap.retrieve( pow2(logsize), limit );
    DBG::outln(DBG::Frame, "FM alloc: ", (ptr_t)pow2(logsize), " -> ", (ptr_t)addr);
    return addr;
  }

  bool alloc( laddr addr, size_t length ) {
    DBG::outln(DBG::Frame, "FM alloc: ", (ptr_t)addr, '/', (ptr_t)length);
    return bmap.remove(addr,length);
  }

  bool release( laddr addr, size_t length ) {
    DBG::outln(DBG::Frame, "FM release: ", (ptr_t)addr, '/', (ptr_t)length);
    return bmap.insert(addr, length);
  }
};

extern inline ostream& operator<<(ostream& os, const FrameManager& fm) {
  os << fm.bmap;
  return os;
}

#endif /* _FrameManager_h_ */
