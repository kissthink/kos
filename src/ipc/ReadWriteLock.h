#ifndef _ReadWriteLock_h_
#define _ReadWriteLock_h_ 1

#include "ipc/BlockingSync.h"
#include "ipc/CV.h"

#include <atomic>

class RwMutex {
protected:
  std::atomic<mword> readers;
  std::atomic<bool> writer;
  std::atomic<bool> locked;
  mword readWaiters;
  mword writeWaiters;
  Mutex mtx;
  CV readerQueue;
  CV writerQueue;
public:
  RwMutex() : readers(0), writer(false), locked(false), readWaiters(0), writeWaiters(0) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RwMutex)); }
  virtual void readAcquire() {
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
  virtual void readRelease() {
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
  virtual void writeAcquire() {
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
  virtual void writeRelease() {
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
  virtual bool tryReadAcquire() {
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
  virtual bool tryWriteAcquire() {
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
  virtual bool tryUpgrade() {
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
  virtual void downGrade() {
    mtx.acquire();
    KASSERT1(writer, "downgrading non-writelock");
    readers = 1;
    writer = false;
    readerQueue.broadcast(); // other blocked readers may run, too.
    mtx.release();
  }
  virtual bool isExclusive() {
    return writer;
  }
  virtual bool isLocked() {
    return locked;
  }
};

class RecursiveRwMutex : public RwMutex {
  mword recurse;
  Thread* owner;
public:
  RecursiveRwMutex() : RwMutex(), recurse(0), owner(nullptr) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr,sizeof(RecursiveRwMutex)); }
  void writeAcquire() {
    mtx.acquire();
    if (writer && (owner == Processor::getCurrThread())) {
      recurse += 1;
      mtx.release();
      return;
    }
    while (readers || (writer && owner != Processor::getCurrThread())) {
      writeWaiters += 1;
      writerQueue.wait(mtx);
      writeWaiters -= 1;
    }
    writer = true;  // don't need to check for recursive case here.
    locked = true;
    owner = Processor::getCurrThread();
    mtx.release();
  }
  void writeRelease() {
    mtx.acquire();
    KASSERT1(locked, "releasing an unlocked mutex");
    KASSERT1(!readers, "releasing read lock");
    KASSERT1(owner == Processor::getCurrThread(), "releasing lock not owned");
    if (recurse > 0) {
      recurse -= 1;
      mtx.release();
      return;
    }
    writer = false;
    locked = false;
    owner = nullptr;
    if (readWaiters) {
      readerQueue.broadcast();  // give readers first try
    } else if (writeWaiters) {
      writerQueue.signal();
    }
    mtx.release();
  }
  bool tryWriteAcquire() {
    mtx.acquire();
    if (!locked || (writer && owner == Processor::getCurrThread())) {
      if (writer) {
        recurse += 1;
        mtx.release();
        return true;
      }
      writer = true;
      locked = true;
      owner = Processor::getCurrThread();
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
      owner = Processor::getCurrThread();
      mtx.release();
      return true;
    }
    mtx.release();
    return false;
  }
  void downGrade() {
    mtx.acquire();
    KASSERT1(writer, "downgrading non-writelock");
    KASSERT1(recurse == 0, "downgrading recursive lock");
    readers = 1;
    writer = false;
    owner = nullptr;
    readerQueue.broadcast();  // other blocked readers by run, too.
    mtx.release();
  }
};

#endif /* _ReadWriteLock_h_ */
