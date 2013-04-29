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
#ifndef _FrameManager_h_
#define _FrameManager_h_ 1

#include "util/BuddyMap.h"
#include "util/Debug.h"
#include "kern/KernelVM.h"

#include <set>

class FileCache {
  friend std::ostream& operator<<(std::ostream&, const FileCache&);
  static const mword min = pagesizebits<1>();
  using BuddySet = InPlaceSet<vaddr,min,kernelBase>;
  BuddyMap<min,framebits,BuddySet> availableMemory;
public:
  laddr reclaim( size_t size ) { return topaddr; }
};

class FrameManager {
  friend std::ostream& operator<<(std::ostream&, const FrameManager&);
  friend class Machine;
  friend class Multiboot;
  friend class AddressSpace;
  friend class PageManager;

  FileCache fc;

  using BuddySet = std::set<vaddr,std::less<vaddr>,KernelAllocator<vaddr>>;
  BuddyMap<pagesizebits<1>(),framebits,BuddySet> availableMemory;

  template<bool limit = false>
  laddr alloc( size_t size, vaddr upperlimit = mwordlimit ) {
    KASSERT( aligned(size, pagesize<1>()), size );
    vaddr addr = availableMemory.retrieve<limit>(size, upperlimit);
    if (!limit && likely(addr == topaddr)) addr = fc.reclaim(size);
    DBG::outln(DBG::Frame, "FM alloc: ", FmtHex(size), " -> ", FmtHex(addr));
    return addr;
  }

  bool reserve( laddr addr, size_t size ) {
    DBG::outln(DBG::Frame, "FM reserve: ", FmtHex(addr), '/', FmtHex(size));
    KASSERT( aligned(addr, pagesize<1>()), addr );
    KASSERT( aligned(size, pagesize<1>()), size );
    return availableMemory.remove(addr,size);
  }

  bool release( laddr addr, size_t size ) {
    DBG::outln(DBG::Frame, "FM release: ", FmtHex(addr), '/', FmtHex(size));
    KASSERT( aligned(addr, pagesize<1>()), addr );
    KASSERT( aligned(size, pagesize<1>()), size );
    return availableMemory.insert(addr, size);
  }

  FileCache& getFC() { return fc; }
  
};

#endif /* _FrameManager_h_ */
