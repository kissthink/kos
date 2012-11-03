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
#include "util/Log.h"
#include "kern/Kernel.h"
#include "kern/KernelHeap.h"
#include "kern/AddressSpace.h"

char KernelHeap::mem[sizeof(KernelHeap::State)];

void KernelHeap::init(mword start, mword end) {
  // need one cache block for address space cache
  KASSERT( end >= start + 2 * cacheSize, end - start );
  new (mem) State;
  state()->buddyMap.insert(start, end - start);
  // TODO: preload AddressSpace cache, one element per buddyLevel
}

vaddr KernelHeap::allocInternal(size_t size) {
  KASSERT( aligned(size, cacheSize), size );
  LockObject<SpinLock> lo(state()->buddyMapLock);
  for (;;) {
    vaddr addr = state()->buddyMap.retrieve(size);
  if (addr != illaddr) return addr;
    addr = kernelSpace.allocPages<dpl>(size);
    KASSERT( addr != illaddr, "out of memory?" );
    bool check = state()->buddyMap.insert(addr, align_up(size, pageSize));
    KASSERT( check, addr );
    // TODO: check AddressSpace cache; if necessary allocate another page.
  }
}

void KernelHeap::releaseInternal(vaddr p, size_t size) {
  KASSERT( aligned(size, cacheSize), size );
  LockObject<SpinLock> lo(state()->buddyMapLock);
  state()->buddyMap.insert(p, size);
  // TODO: check AddressSpace cache before releasing pages
  for (;;) {
    vaddr addr = state()->buddyMap.retrieve(pageSize);
    if (addr == illaddr) break;
    bool check = kernelSpace.releasePages<dpl>(p, size);
    KASSERT(check, p);
  }
}
