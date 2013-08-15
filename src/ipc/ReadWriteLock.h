#ifndef _ReadWriteLock_h_
#define _ReadWriteLock_h_ 1

#include "ipc/BlockingSync.h"
#include "ipc/CV.h"

#include <atomic>

class RwMutex {
  std::atomic<mword> readers;
  std::atomic<bool> writer;
  std::atomic<bool> locked;
  mword readWaiters;
  mword writeWaiters;
  Mutex mtx;
  CV readerQueue;
  CV writerQueue;
public:
  RwMutex() : readers(0), writer(false) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RwMutex)); }
  void readAcquire() {
    mtx.acquire();
    while (writer) {
      readWaiters += 1;
      readerQueue.wait(mtx);
      readWaiters -= 1;
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
      writerQueue.signal(); // readers shouldn't need to be woken up
    }
    mtx.release();
  }
  void writeAcquire() {
    mtx.acquire();
    while (locked) {
      writeWaiters += 1;
      writerQueue.wait(mtx);
      writeWaiters -= 1;
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
    if (readWaiters) {
      readerQueue.broadcast();  // give readers first try
    } else if (writeWaiters) {
      writerQueue.signal();
    }
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
    readerQueue.broadcast(); // other blocked readers may run, too.
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
