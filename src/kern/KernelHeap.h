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
#ifndef _KernelHeap_h
#define _KernelHeap_h 1

#include "extern/stl/mod_set"
#include "util/BuddyMap.h"
#include "util/CacheHeap.h"
#include "util/SpinLock.h"
#include "mach/bitstrops.h"

#include <queue>

template<typename T> class KernelAllocator;

// allocations of less than cacheSize are routed through a caching heap
class KernelHeap {
  static const size_t dpl = 2;
  static const size_t cacheLimit = pagesizebits<1>();
  static const size_t cacheSize = pow2(cacheLimit);
  static const size_t pageLimit = pagesizebits<dpl>();
  static const size_t pageSize = pow2(pageLimit);

  static_assert(pageLimit >= cacheLimit, "pageLimit less than cacheLimit");

  struct BlockHelper {
    static vaddr alloc(size_t size) { return allocInternal(size); }
  };
  struct State {
    BlockCacheHeap<BlockHelper,cacheSize,SpinLock> cacheHeaps[cacheLimit];
    BuddyMap<cacheLimit,pageLimit+1,InPlaceSet<vaddr>,KernelAllocator<vaddr>> buddyMap;
    volatile SpinLock buddyMapLock;
  };
  static char mem[sizeof(State)];
  static constexpr State* state() { return (State*)mem; }

  static vaddr allocInternal(size_t size);
  static void releaseInternal(vaddr p, size_t size);

public:
  KernelHeap() = delete;                             // no creation
  KernelHeap(const KernelHeap&) = delete;            // no copy
  KernelHeap& operator=(const KernelHeap&) = delete; // no assignment

  static void init(mword start, mword end);

  static vaddr alloc(size_t size) {
    if likely(size < cacheSize) {
      size_t index = ceilinglog2(size);
      return state()->cacheHeaps[index].alloc(pow2(index));
    } else {
      return allocInternal(size);
    }
  }

  static void release(vaddr p, size_t size) {
    if likely(size < cacheSize) {
      size_t index = ceilinglog2(size);
      state()->cacheHeaps[index].release(p, pow2(index));
    } else {
      releaseInternal(p, size);
    }
  }

  typedef BackedCacheHeap<KernelHeap> Cache;
  static Cache* createCache(size_t s) { return new Cache(s); }
  static void destroyCache(Cache* c) { delete c; }

  static ptr_t malloc(size_t size) { return (ptr_t)alloc(size); }
  static void free(ptr_t p, size_t size) { release(vaddr(p),size); }

};

template<typename T> class KernelAllocator : public std::allocator<T> {
public:
  template<typename U> struct rebind { typedef KernelAllocator<U> other; };
  KernelAllocator() = default;
  KernelAllocator(const KernelAllocator& x) = default;
  template<typename U> KernelAllocator (const KernelAllocator<U>& x) : std::allocator<T>(x) {}
  ~KernelAllocator() = default;
  T* allocate(size_t n, const void* = 0) {
    return static_cast<T*>(KernelHeap::malloc(n * sizeof(T)));
  }
  void deallocate(T* p, size_t s) {
    KernelHeap::free(p, s * sizeof(T));
  }
};

template <typename T> using KernelQueue = std::queue<T,std::deque<T,KernelAllocator<T>>>;

#endif /* _KernelHeap_h */
