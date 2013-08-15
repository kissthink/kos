#ifndef _ReadWriteLock_h_
#define _ReadWriteLock_h_ 1

#include "ipc/BlockingSync.h"
#include "ipc/CV.h"

#include <atomic>

class RwMutex {
  std::atomic<mword> readers;
  std::atomic<bool> writer;
  std::atomic<bool> locked;
  Mutex mtx;
  CV cond;
public:
  RwMutex() : readers(0), writer(false) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RwMutex)); }
  void readAcquire() {
    mtx.acquire();
    while (writer) {
      cond.wait(mtx);
    }
    readers += 1;
    locked = true;
    mtx.release();
  }
  void readRelease() {
    mtx.acquire();
    KASSERT1(locked, "releasing an unlocked mutex");
    KASSERT1(!writer, "releasing write lock");
    readers -= 1;
    if (readers == 0) {
      locked = false;
      cond.broadcast(); // wakeup all blocked readers/writers
    }
    mtx.release();
  }
  void writeAcquire() {
    mtx.acquire();
    while (locked) {
      cond.wait(mtx);
    }
    writer = true;
    locked = true;
    mtx.release();
  }
  void writeRelease() {
    mtx.acquire();
    KASSERT1(locked, "releasing an unlocked mutex");
    KASSERT1(!readers, "releasing read lock");
    writer = false;
    locked = false;
    cond.broadcast(); // wakeup all blocked readers/writers
    mtx.release();
  }
  bool tryReadAcquire() {
    mtx.acquire();
    if (!writer) {
      readers += 1;
      locked = true;
      mtx.release();
      return true;
    }
    mtx.release();
    return false;
  }
  bool tryWriteAcquire() {
    mtx.acquire();
    if (!locked) {
      writer = true;
      locked = true;
      mtx.release();
      return true;
    }
    mtx.release();
    return false;
  }
  bool tryUpgrade() {
    mtx.acquire();
    if (readers == 1) {
      readers = 0;
      writer = true;
      mtx.release();
      return true;
    }
    mtx.release();
    return false;
  }
  void downGrade() {
    mtx.acquire();
    KASSERT1(writer, "downgrading non-writelock");
    readers = 1;
    writer = false;
    cond.broadcast(); // wakeup all blocked readers (but it's okay to wake up writers, too)
    mtx.release();
  }
  bool isExclusive() {
    return writer;
  }
  bool isLocked() {
    return locked;
  }
};

#endif /* _ReadWriteLock_h_ */
