#ifndef _CondVar_h_
#define _CondVar_h_ 1

#include "extern/stl/mod_list"
#include "mach/Processor.h"
#include "kern/Kernel.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"
#include "ipc/SpinLock.h"
#include "ipc/BlockingSync.h"

class BlockingSyncMutex {
  InPlaceList<Thread*> threadQueue;
  void* currMutex;
protected:
  SpinLock lk;
  ~BlockingSyncMutex() {
    ScopedLock<> lo(lk);
    while (!threadQueue.empty()) resume();
  }
  void suspend(Mutex* mtx) {
    if (!currMutex) currMutex = mtx;
    KASSERT1(mtx == currMutex, "wait queue is associated with different mutex");
    threadQueue.push_back(Processor::getCurrThread());
    kernelScheduler.suspend(lk);
  }
  bool suspend(RwMutex* mtx) {
    if (!currMutex) currMutex = mtx;
    KASSERT1(mtx == currMutex, "wait queue is associated with different mutex");
    threadQueue.push_back(Processor::getCurrThread());
    kernelScheduler.suspend(lk);
    return mtx->isExclusive();
  }
  Thread* resume() {
    Thread* t = threadQueue.front();
    threadQueue.pop_front();
    if (threadQueue.empty()) currMutex = nullptr; // mutex is not associated now
    kernelScheduler.start(*t);
    return t;
  }
  void resumeAll() {
    while (!threadQueue.empty()) resume();
    currMutex = nullptr;
  }
  bool waiting() {
    return !threadQueue.empty();
  }
};

// Remember which threads are blocked on which mutex
class ConditionVar : public BlockingSyncMutex {
  SpinLock lk;
public:
  ConditionVar() {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(ConditionVar)); }
  void wait(Mutex* mutex, bool acquire = true) {
    ScopedLock<> lo(lk);
    KASSERT1(mutex->locked(), "passed un-locked mutex in cond var");
    suspend(mutex);
    if (acquire) mutex->acquire();   // FreeBSD's cv_wait_unlock() do not reacquire lock on wake up
  }
  void wait(RwMutex* mutex, bool acquire = true) {
    ScopedLock<> lo(lk);
    KASSERT1(mutex->locked(), "passed un-locked rw mutex in cond var");
    bool exclusive = suspend(mutex);
    if (acquire) {
      if (exclusive) mutex->writeAcquire();
      else mutex->readAcquire();
    }
  }
  void signal() {
    ScopedLock<> lo(lk);
    if (waiting()) resume();
  }
  void broadcast() {
    ScopedLock<> lo(lk);
    if (waiting()) resumeAll();
  }
};

#endif /* _CondVar_h_ */
