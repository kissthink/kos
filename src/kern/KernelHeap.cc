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
  KASSERT( end >= start + 2 * cacheSize, end - start );
  new (mem) State;
  state->buddyMap.insert(start, end - start);
  state->buddyMapSize += end - start;
}

vaddr KernelHeap::allocInternal(size_t size) {
  KASSERT( aligned(size, cacheSize), size );
  LockObject<SpinLock> lo(state->buddyMapLock);
  // low watermark: keep one cacheSize block available
  if (state->buddyMapSize < cacheSize + size) {
    vaddr newmem = kernelSpace.mapPages<dpl,true>(align_up(size, pageSize));
    KASSERT( newmem != illaddr, "out of memory?" );
    bool check = state->buddyMap.insert(newmem, align_up(size, pageSize));
    KASSERT( check, newmem );
    state->buddyMapSize += align_up(size, pageSize);
  }
  vaddr addr = state->buddyMap.retrieve(size);
  KASSERT( addr != illaddr, "internal error" );
  state->buddyMapSize -= size;
  return addr;
}

void KernelHeap::releaseInternal(vaddr p, size_t size) {
  KASSERT( aligned(size, cacheSize), size );
  LockObject<SpinLock> lo(state->buddyMapLock);
  state->buddyMap.insert(p, size);
  state->buddyMapSize += size;
  // high watermark: keep one pageSize block available
  vaddr freeSize = size < pageSize ? pageSize : align_down(size, pageSize);
  while (state->buddyMapSize >= pageSize + freeSize) {
    vaddr addr = state->buddyMap.retrieve(freeSize);
    if (addr == illaddr) break;
    state->buddyMapSize -= freeSize;
    bool check = kernelSpace.unmapPages<dpl,true>(addr, freeSize);
    KASSERT( check, addr );
  }
}
