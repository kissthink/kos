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
#ifndef _SpinLock_h_
#define _SpinLock_h_ 1

#include "mach/Processor.h"
#include "kern/globals.h"

class SpinLock {
  bool locked;
  void acquireInternal() volatile {
    while (__atomic_test_and_set(&locked, __ATOMIC_SEQ_CST))
      while (locked) Pause();
  }
  void releaseInternal() volatile {
    __atomic_clear(&locked, __ATOMIC_SEQ_CST);
  }
public:
  SpinLock() : locked(false) {}
  inline ptr_t operator new(std::size_t);
  inline void operator delete(ptr_t ptr);
  static constexpr size_t size() {
    return sizeof(SpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(SpinLock);
  }
  void acquire() volatile {
    Processor::disableInterrupts();
    acquireInternal();
  }
  void release() volatile {
    releaseInternal();
    Processor::enableInterrupts();
  }
  void acquireISR() volatile {
    Processor::incLockCount();
    acquireInternal();
  }
  void releaseISR() volatile {
    releaseInternal();
    Processor::decLockCount();
  }
};

inline ptr_t SpinLock::operator new(std::size_t) {
  return ::operator new(SpinLock::size());
}
inline void SpinLock::operator delete(ptr_t ptr) {
  globaldelete(ptr, SpinLock::size());
}

class NoLock {
public:
  void acquire() volatile {}
  void release() volatile {}
  void acquireISR() volatile {}
  void releaseISR() volatile {}
};

template <class Lock>
class LockObject {
  volatile Lock& lk;
public:
  LockObject(volatile Lock& lk) : lk(lk) { lk.acquire(); }
  ~LockObject() { lk.release(); }
};

template <class Lock>
class LockObjectISR {
  volatile Lock& lk;
public:
  LockObjectISR(volatile Lock& lk) : lk(lk) { lk.acquireISR(); }
  ~LockObjectISR() { lk.releaseISR(); }
};

#endif /* _SpinLock_h_ */
