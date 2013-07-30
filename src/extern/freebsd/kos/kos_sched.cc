#include <cstdint>
#include "ipc/BlockingSync.h"
#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"
#include "kern/Scheduler.h"

#include "extern/stl/mod_list"
#include <map>
using namespace std;

extern Scheduler kernScheduler;

typedef InPlaceList<Thread*> ThreadList;
static map<void*, InPlaceList<Thread*>> sleepQueue;
static SpinLock lk;

static void acquireLock(void* lock, int lockType) {
  if (lock == nullptr) return;
  switch (lockType) {
    case 0: {
      Mutex* mtx = static_cast<Mutex*>(lock);
      mtx->acquire();
    } break;
    case 1: {
      RwMutex* mtx = static_cast<RwMutex*>(lock);
      mtx->readAcquire();
    } break;
    case 2: {
      RwMutex* mtx = static_cast<RwMutex*>(lock);
      mtx->writeAcquire();
    } break;
    case 3: {
      SpinLock* lk = static_cast<SpinLock*>(lock);
      lk->acquire();
    } break;
    default: ABORT1("invalid argument to acquireLock()");
  }
}

static int releaseLock(void* lock, int lockType) {
  if (lock == nullptr) return -1;
  int newLockType = lockType;
  switch (lockType) {
    case 0: {
      Mutex* mtx = static_cast<Mutex*>(lock);
      mtx->release();
    } break;
    case 1: {
      RwMutex* mtx = static_cast<RwMutex*>(lock);
      if (mtx->isExclusive()) {
        mtx->writeRelease();
        newLockType = 2;
      } else {
        mtx->readRelease();
      }
    } break;
    case 2: {
      SpinLock* lk = static_cast<SpinLock*>(lock);
      lk->release();
    } break;
    default: ABORT1("invalid lockType to releaseLock()");
  }
  return newLockType;
}
      
int kos_sleep(void* ident, void* lock, int lockType, int pri, int timeout) {
  lk.acquire();
  KASSERT0(ident);
  int error = 0;
  ThreadList& tlist = sleepQueue[ident];
  Thread* t = Processor::getCurrThread();
  tlist.push_back(t);
  // release lock if not NULL
  int newLockType = releaseLock(lock, lockType);
  if (timeout) {
    error = t->sleep(timeout, lk);    // reacquire lk on wakeup
    tlist.remove(t);                  // kos_wakeup() can race and remove before this runs; however, still consider it timedout
    lk.release();
  } else {
    kernScheduler.suspend(lk);
  }
  // acquire lock if not NULL
  acquireLock(lock, newLockType);
  return error;
}

void kos_wakeup(void* chan) {
  ScopedLock<> lo(lk);
  KASSERT0(chan);
  if (sleepQueue.count(chan) == 0) return;
  ThreadList& tlist = sleepQueue[chan];
  auto it = begin(tlist);
  while (it != end(tlist)) {
    kernScheduler.start(**it);
    tlist.erase(it++);
  }
  sleepQueue.erase(chan);
}

void kos_wakeup_one(void* chan) {
  ScopedLock<> lo(lk);
  KASSERT0(chan);
  if (sleepQueue.count(chan) == 0) return;
  ThreadList& tlist = sleepQueue[chan];
  kernScheduler.start(*tlist.front());
  tlist.pop_front();
  if (tlist.empty()) sleepQueue.erase(chan);
}
