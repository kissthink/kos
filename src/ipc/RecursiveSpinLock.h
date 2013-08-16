#ifndef _RecursiveSpinLock_h_
#define _RecursiveSpinLock_h_ 1

#include "ipc/SpinLock.h"
#include "mach/Processor.h"

class Thread;
class RecursiveSpinLock {
  Thread* owner;
  int recurse;
  bool locked;
  SpinLock lk;
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
  RecursiveSpinLock() : owner(nullptr), recurse(0), locked(false) {}
  ptr_t operator new(size_t) { return ::operator new(size()); }
  void operator delete(ptr_t ptr) { globaldelete(ptr, size()); }
  static constexpr size_t size() {
    return sizeof(RecursiveSpinLock) < sizeof(vaddr) ? sizeof(vaddr) : sizeof(RecursiveSpinLock);
  }
  void acquire() volatile {
    lk.acquire();
    if (owner && owner == Processor::getCurrThread()) {
      recurse += 1;
      lk.release();
      return;
    }
    Processor::incLockCount();
    lk.release(); // release before possible spinning
    acquireInternal();
    owner = Processor::getCurrThread();
  }
  void release() volatile {
    ScopedLock<> lo(lk);
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    if (recurse) {
      recurse -= 1;
      return;
    }
    releaseInternal();
    owner = nullptr;
    Processor::decLockCount();
  }
  bool tryAcquire() volatile {
    ScopedLock<> lo(lk);
    if (owner) {
      if (owner != Processor::getCurrThread())  // another thread is holding the lock
        return false;
      else {
        recurse += 1;   // I was holding the lock
        return true;
      }
    }
    Processor::incLockCount();
    bool acquired = tryAcquireInternal();
    if (acquired) {
      owner = Processor::getCurrThread(); // update owner info
      return true;
    }
    Processor::decLockCount();  // failed to acquire
    return false;
  }
  bool isOwner() volatile {
    ScopedLock<> lo(lk);
    return owner == Processor::getCurrThread();
  }
};

#endif /* _RecursiveSpinLock_h_ */
