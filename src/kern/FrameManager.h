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
  BuddyMap<pagesizebits<1>(),framebits,BuddySet> availPM;

  template<bool limit = false>
  laddr alloc( size_t size, vaddr upperlimit = mwordlimit ) {
    KASSERT( aligned(size, pagesize<1>()), size );
    vaddr addr = availPM.retrieve<limit>(size, upperlimit);
    DBG::outln(DBG::Frame, "FM alloc: ", (ptr_t)size, " -> ", (ptr_t)addr);
    return addr;
  }

  bool reserve( laddr addr, size_t size ) {
    DBG::outln(DBG::Frame, "FM reserve: ", (ptr_t)addr, '/', (ptr_t)size);
    KASSERT( aligned(size, pagesize<1>()), size );
    return availPM.remove(addr,size);
  }

  bool release( laddr addr, size_t size ) {
    DBG::outln(DBG::Frame, "FM release: ", (ptr_t)addr, '/', (ptr_t)size);
    KASSERT( aligned(size, pagesize<1>()), size );
    return availPM.insert(addr, size);
  }
};

extern inline ostream& operator<<(ostream& os, const FrameManager& fm) {
  os << fm.availPM;
  return os;
}

#endif /* _FrameManager_h_ */
