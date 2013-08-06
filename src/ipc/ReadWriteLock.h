#ifndef _ReadWriteLock_h_
#define _ReadWriteLock_h_ 1

#include "extern/stl/mod_list"
#include "kern/Kernel.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"
#include "ipc/BlockingSync.h"
#include "ipc/CondVar.h"
#include "ipc/SpinLock.h"

#include <atomic>

class RwMutex {
  std::atomic<mword> readers;
  std::atomic<bool> writer;
  Mutex mtx;
  ConditionVar cond;
public:
  RwMutex() : readers(0), writer(false) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RwMutex)); }
  void readAcquire();
  void readRelease();
  void writeAcquire();
  void writeRelease();
  bool tryReadAcquire();
  bool tryWriteAcquire();
  bool tryUpgrade();
  void downGrade();
  bool isExclusive() {
    return writer;
  }
  bool locked() {
    return readers || writer;
  }
};

#endif /* _ReadWriteLock_h_ */
