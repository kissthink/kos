/******************************************************************************
    Copyright 2013 Martin Karsten

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
#ifndef _KernelVM_h_
#define _KernelVM_h_ 1

#include "extern/stl/mod_set"
#include "util/BuddyMap.h"
#include "util/SpinLock.h"
#include "mach/Memory.h"

#include <memory>

template<typename T> class KernelAllocator;

class KernelVM {
  friend std::ostream& operator<<(std::ostream&, const KernelVM&);

  static const mword min = pagesizebits<1>();
  static const mword max = ceilinglog2_c(kernelRange);
  static const mword dpl = 2;
  using BuddySet = InPlaceSet<vaddr,min,kernelBase>;
  BuddyMap<min,max,BuddySet> availableMemory;
  SpinLock lock;
  void* kmspace;

  vaddr allocInternal(size_t size);
  void releaseInternal(vaddr p, size_t size);

  KernelVM(const KernelVM&) = delete;                  // no copy
  const KernelVM& operator=(const KernelVM&) = delete; // no assignment

  inline void checkExpand(size_t size);

public:
  KernelVM() {} // don't do anything, since constructor is called *after* init
  void init(vaddr start, vaddr end);
  void addMemory(vaddr start, vaddr end);
  vaddr alloc(size_t size) {
    if likely(size < pow2(min)) {
      return (vaddr)malloc(size);
    } else {
      return allocInternal(size);
    }
  }
  void release(vaddr p, size_t size) {
    if likely(size < pow2(min)) {
      return free((ptr_t)p);
    } else {
      releaseInternal(p,size);
    }
  }
  ptr_t malloc(size_t s);
  void free(ptr_t p);
};

extern KernelVM kernelVM;

template<typename T> class KernelAllocator : public std::allocator<T> {
public:
  template<typename U> struct rebind { typedef KernelAllocator<U> other; };
  KernelAllocator() = default;
  KernelAllocator(const KernelAllocator& x) = default;
  template<typename U> KernelAllocator (const KernelAllocator<U>& x) : std::allocator<T>(x) {}
  ~KernelAllocator() = default;
  T* allocate(size_t n, const void* = 0) {
    return (T*)(kernelVM.alloc(n * sizeof(T)));
  }
  void deallocate(T* p, size_t s) {
    kernelVM.release((vaddr)p, s * sizeof(T));
  }
};

#endif /* _KernelVM_h_ */
