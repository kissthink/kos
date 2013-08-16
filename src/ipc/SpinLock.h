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
#ifndef _SpinLock_h_
#define _SpinLock_h_ 1

#include "mach/Processor.h"
#include "kern/globals.h"
#include "kern/Thread.h"

class SpinLock {
  bool locked;
  void acquireInternal() volatile {
    while (__atomic_test_and_set(&locked, __ATOMIC_SEQ_CST))
      while (locked) Pause();
  }
  void releaseInternal() volatile {
    __atomic_clear(&locked, __ATOMIC_SEQ_CST);
  }
  bool tryAcquireInternal() volatile {
    return !__atomic_test_and_set(&locked, __ATOMIC_SEQ_CST);
  }
public:
  SpinLock() : locked(false) {}
  ptr_t operator new(size_t) { return ::operator new(size()); }
  void operator delete(ptr_t ptr) { globaldelete(ptr, size()); }
  static constexpr size_t size() {
    return sizeof(SpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(SpinLock);
  }
  void acquire() volatile {
    Processor::incLockCount();
    acquireInternal();
  }
  void release() volatile {
    releaseInternal();
    Processor::decLockCount();
  }
  bool tryAcquire() volatile {
    Processor::incLockCount();
    bool success = tryAcquireInternal();
    if (!success) Processor::decLockCount();
    return success;
  }
};

class NoLock {
public:
  void acquire() volatile {}
  void release() volatile {}
  bool tryAcquire() volatile { return true; }
};

template <typename Lock = SpinLock>
class ScopedLock {
  volatile Lock& lk;
public:
  ScopedLock(volatile Lock& lk) : lk(lk) { lk.acquire(); }
  ~ScopedLock() { lk.release(); }
};

#endif /* _SpinLock_h_ */
