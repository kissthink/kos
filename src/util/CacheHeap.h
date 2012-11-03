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
#ifndef _CacheHeap_h_
#define _CacheHeap_h_ 1

#include "util/BlockHeap.h"
#include "util/Log.h"
#include "util/SpinLock.h"
#include "kern/globals.h"

/* CacheHeap provides a caching front end to another heap. Freed elements are
 * not returned to the base heap, but kept in a free stack and reused. */

template <typename Lock = NoLock>
class CacheHeap {
  volatile Lock lock;
  vaddr freeStack;
  size_t freeCount;

public:
  CacheHeap() : freeStack(illaddr), freeCount(0) {}

  vaddr alloc(size_t size) {
    KASSERT( size >= sizeof(vaddr), size );
    LockObject<Lock> lo(lock);
    vaddr result = freeStack;
    if likely(freeStack != illaddr) {
      freeStack = *(vaddr*)freeStack;
      freeCount -= 1;
    }
    return result;
  }

  void release(vaddr p, size_t size) {
    KASSERT( size >= sizeof(vaddr), size );
    LockObject<Lock> lo(lock);
    *(vaddr*)p = freeStack;
    freeStack = p;
    freeCount += 1;
  }
};

template <typename BaseHeap, typename Lock = NoLock>
class BackedCacheHeap : public CacheHeap<Lock> {
  size_t size;
public:
  BackedCacheHeap(size_t s) : size(s) {}
  ~BackedCacheHeap() { purge(); }
  void operator delete(void* ptr) {
    globaldelete(ptr, sizeof(BackedCacheHeap));
  }

  size_t getSize() { return size; }

  vaddr alloc() {
    vaddr addr = CacheHeap<Lock>::alloc(size);
    if (addr != illaddr) return addr;
    return BaseHeap::alloc(size);
  }
  void release(vaddr p) {
    CacheHeap<Lock>::release(p, size);
  }
  void purge() {
    for (;;) {
      vaddr addr = CacheHeap<Lock>::alloc(size);
      if (addr == illaddr) return;
      BaseHeap::release(addr, size);
    }
  }
};

template <typename BaseHeap, size_t allocSize, typename Lock = NoLock>
class BlockCacheHeap : public CacheHeap<Lock> {
  BlockHeap<BaseHeap,allocSize,Lock> blockHeap;
public:
  vaddr alloc(size_t size) {
    vaddr addr = CacheHeap<Lock>::alloc(size);
    if (addr != illaddr) return addr;
    return blockHeap.alloc(size);
  }
};

#endif /* _CacheHeap_h_ */
