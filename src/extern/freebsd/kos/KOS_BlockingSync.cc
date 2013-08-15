#include "kos/BlockingSync.h"

// KOS
#include <cstdint>
#include "ipc/BlockingSync.h"
#include "ipc/SpinLock.h"
#include "ipc/ReadWriteLock.h"
#include "util/Output.h"

void KOS_Lock_Init(void** kos_lock, int spin, int recurse) {
  if (spin) {
    if (recurse) *kos_lock = new RecursiveSpinLock();
    else *kos_lock = new SpinLock();
  } else {
    if (recurse) *kos_lock = new RecursiveMutex();
    else *kos_lock = new Mutex();
  }
}

void KOS_SpinLock_Lock(void* kos_lock) {
  SpinLock* lock = static_cast<SpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->acquire();
}

void KOS_SpinLock_UnLock(void* kos_lock) {
  SpinLock* lock = static_cast<SpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->release();
}

void KOS_RSpinLock_Lock(void* kos_lock) {
  RecursiveSpinLock* lock = static_cast<RecursiveSpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->acquire();
}

void KOS_RSpinLock_UnLock(void* kos_lock) {
  RecursiveSpinLock* lock = static_cast<RecursiveSpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->release();
}

void KOS_Mutex_Lock(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->acquire();
}

int KOS_Mutex_TryLock(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryAcquire() ? 1 : 0;
}

void KOS_Mutex_UnLock(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->release();
}

void KOS_RMutex_Lock(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->acquire();
}

int KOS_RMutex_TryLock(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryAcquire() ? 1 : 0;
}

void KOS_RMutex_UnLock(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->release();
}

void KOS_RwMutex_Init(void** kos_lock, int recurse) {
  // TODO recurse
  *kos_lock = new RwMutex();
}

void KOS_RwMutex_RLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->readAcquire();
}

void KOS_RwMutex_WLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->writeAcquire();
}

void KOS_RwMutex_RUnLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->readRelease();
}

void KOS_RwMutex_WUnLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->writeRelease();
}

int KOS_RwMutex_RTryLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryReadAcquire() ? 1 : 0;
}

int KOS_RwMutex_WTryLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryWriteAcquire() ? 1 : 0;
}

int KOS_RwMutex_TryUpgrade(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryUpgrade() ? 1 : 0;
}

void KOS_RwMutex_Downgrade(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->downGrade();
}

enum class MutexType {
  MUTEX,
  RECURSIVE_MUTEX,
  SPINLOCK,
  RECURSIVE_SPINLOCK,
};

struct LockWrapper {
  LockWrapper(ptr_t lock, const MutexType& type) : lock(lock), type(type), owner(nullptr) {}
  ptr_t lock;
  MutexType type;
  Thread* owner;
  virtual void acquire() = 0;
  virtual void release() = 0;
  virtual bool tryAcquire() = 0;
};

struct MutexWrapper : public LockWrapper {
  MutexWrapper(Mutex* mtx) : LockWrapper(mtx, MutexType::MUTEX) {}
  void acquire() {
    Mutex* mtx = static_cast<Mutex*>(lock);
    DBG::outln(DBG::Basic, "acquiring mutex:", FmtHex(mtx), " thread:", FmtHex(Processor::getCurrThread()));
    mtx->acquire();
    owner = Processor::getCurrThread();
    DBG::outln(DBG::Basic, "acquired mutex:", FmtHex(mtx), " thread:", FmtHex(Processor::getCurrThread()));
  }
  void release() {
    Mutex* mtx = static_cast<Mutex*>(lock);
    owner = nullptr;
    DBG::outln(DBG::Basic, "releasing mutex:", FmtHex(mtx), " thread:", FmtHex(Processor::getCurrThread()));
    mtx->release();
    DBG::outln(DBG::Basic, "released mutex:", FmtHex(mtx), " thread:", FmtHex(Processor::getCurrThread()));
  }
  bool tryAcquire() {
    Mutex* mtx = static_cast<Mutex*>(lock);
    bool acquired = mtx->tryAcquire();
    if (acquired)
      owner = Processor::getCurrThread();
    return acquired;
  }
};

struct RecursiveMutexWrapper : public LockWrapper {
  int recurse;
  SpinLock lk;
  RecursiveMutexWrapper(RecursiveMutex* mtx) : LockWrapper(mtx, MutexType::RECURSIVE_MUTEX), recurse(0) {}
  void acquire() {
    RecursiveMutex* mtx = static_cast<RecursiveMutex*>(lock);
    mtx->acquire();
    ScopedLock<> lo(lk);
    recurse += 1;
    if (recurse == 1)
      owner = Processor::getCurrThread();
  }
  void release() {
    RecursiveMutex* mtx = static_cast<RecursiveMutex*>(lock);
    ScopedLock<> lo(lk);
    recurse -= 1;
    if (recurse == 0)
      owner = nullptr;
    mtx->release();
  }
  bool tryAcquire() {
    RecursiveMutex* mtx = static_cast<RecursiveMutex*>(lock);
    bool acquired = mtx->tryAcquire();
    if (acquired) {
      lk.acquire();
      recurse += 1;
      if (recurse == 1)
        owner = Processor::getCurrThread();
      lk.release();
    }
    return acquired;
  }
};

struct SpinLockWrapper : public LockWrapper {
  SpinLockWrapper(SpinLock* mtx) : LockWrapper(mtx, MutexType::SPINLOCK) {}
  void acquire() {
    SpinLock* lk = static_cast<SpinLock*>(lock);
    lk->acquire();
    DBG::outln(DBG::Basic, "acquired spinlock:", FmtHex(lk));
  }
  void release() {
    SpinLock* lk = static_cast<SpinLock*>(lock);
    lk->release();
    DBG::outln(DBG::Basic, "released spinlock:", FmtHex(lk));
  }
  bool tryAcquire() {
    SpinLock* lk = static_cast<SpinLock*>(lock);
    return lk->tryAcquire();
  }
};

struct RecursiveSpinLockWrapper : public LockWrapper {
  RecursiveSpinLockWrapper(RecursiveSpinLock* mtx) : LockWrapper(mtx, MutexType::RECURSIVE_SPINLOCK) {}
  void acquire () {
    RecursiveSpinLock* lk = static_cast<RecursiveSpinLock*>(lock);
    lk->acquire();
  }
  void release() {
    RecursiveSpinLock* lk = static_cast<RecursiveSpinLock*>(lock);
    lk->release();
  }
  bool tryAcquire() {
    RecursiveSpinLock* lk = static_cast<RecursiveSpinLock*>(lock);
    return lk->tryAcquire();
  }
};

////////////////

static SpinLock lockMapLock;
static map<ptr_t,LockWrapper*,less<ptr_t>,KernelAllocator<pair<ptr_t,LockWrapper*>>> lockMap; 

// mutex/recursive mutex
void KOS_MutexLock(ptr_t mtx, int opts, const char* file, int line) {
  lockMapLock.acquire();
  KASSERT1(mtx, "null mutex");
  KASSERT1(lockMap.count(mtx), "unknown mutex");
  LockWrapper* lw = lockMap[mtx];
  lockMapLock.release();
  if (lw->owner == Processor::getCurrThread())
    KASSERTN(lw->type == MutexType::RECURSIVE_MUTEX || lw->type == MutexType::RECURSIVE_SPINLOCK,
        "recursed on non-recursive mutex @ ", file, ':', line); 

  DBG::outln(DBG::Basic, "KOS_MutexLock:", FmtHex(lw), " @ ", file, ':', line);
  lw->acquire();  // acquire lock!
}

void KOS_MutexUnLock(ptr_t mtx, int opts, const char* file, int line) {
  lockMapLock.acquire();
  KASSERT1(mtx, "null mutex");
  KASSERT1(lockMap.count(mtx), "unknown mutex");
  LockWrapper* lw = lockMap[mtx];
  lockMapLock.release();

  DBG::outln(DBG::Basic, "KOS_MutexUnLock:", FmtHex(lw), " @ ", file, ':', line);
  lw->release();  // release lock!
}

int KOS_MutexTryLock(ptr_t mtx, int opts, const char* file, int line) {
  lockMapLock.acquire();
  KASSERT1(mtx, "null mutex");
  KASSERT1(lockMap.count(mtx), "unknown mutex");
  LockWrapper* lw = lockMap[mtx];
  lockMapLock.release();

  return lw->tryAcquire() ? 1 : 0;
}

// SHOULD MATCH with sys/sys/mutex.h
#define MTX_DEF     0x00000000
#define MTX_SPIN    0x00000001
#define MTX_RECURSE 0x00000004

void KOS_MutexInit(ptr_t mtx, int flags) {
  KASSERT1(mtx, "null mutex");
//  DBG::outln(DBG::Basic, "KOS_MutexInit() - flags:", FmtHex(flags));
  if (flags & MTX_SPIN) {
    if (flags & MTX_RECURSE) {
      RecursiveSpinLock* l = new RecursiveSpinLock;
      lockMapLock.acquire();
      KASSERTN(!lockMap.count(mtx), "KOS lock already exists for mutex ", FmtHex(mtx));
      lockMap[mtx] = new RecursiveSpinLockWrapper(l);
      lockMapLock.release();
    } else {
      SpinLock* l = new SpinLock;
      lockMapLock.acquire();
      KASSERTN(!lockMap.count(mtx), "KOS lock already exists for mutex ", FmtHex(mtx));
      lockMap[mtx] = new SpinLockWrapper(l);
      lockMapLock.release();
    }
  } else {
    if (flags & MTX_RECURSE) {
      RecursiveMutex* m = new RecursiveMutex;
      lockMapLock.acquire();
      KASSERTN(!lockMap.count(mtx), "KOS lock already exists for mutex ", FmtHex(mtx));
      lockMap[mtx] = new RecursiveMutexWrapper(m);
      lockMapLock.release();
    } else {
      Mutex* m = new Mutex;
      lockMapLock.acquire();
      KASSERTN(!lockMap.count(mtx), "KOS lock already exists for mutex ", FmtHex(mtx));
      lockMap[mtx] = new MutexWrapper(m);
      lockMapLock.release();
    }
  }
}
