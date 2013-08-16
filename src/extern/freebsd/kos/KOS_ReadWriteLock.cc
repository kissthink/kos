#include "kos/ReadWriteLock.h"

// KOS
#include <cstdint>
#include "ipc/ReadWriteLock.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"
#include <map>
using namespace std;

enum class RwMutexType {
  DEFAULT,
  RECURSIVE
};

struct RwLockWrapper {
  RwLockWrapper(ptr_t lock, const RwMutexType& type)
    : lock(lock), type(type) {}
  ptr_t lock;
  RwMutexType type;
  virtual void readAcquire() = 0;
  virtual void readRelease() = 0;
  virtual bool tryReadAcquire() = 0;
  virtual void writeAcquire() = 0;
  virtual void writeRelease() = 0;
  virtual bool tryWriteAcquire() = 0;
  virtual bool tryUpgrade() = 0;
  virtual void downgrade() = 0;
  virtual bool owned() = 0;
};

struct RwMutexWrapper : public RwLockWrapper {
  RwMutexWrapper(ptr_t mtx)
    : RwLockWrapper(mtx, RwMutexType::DEFAULT) {}
  void readAcquire() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    mtx->readAcquire();
  }
  void readRelease() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    mtx->readRelease();
  }
  bool tryReadAcquire() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    return mtx->tryReadAcquire();
  }
  void writeAcquire() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    mtx->writeAcquire();
  }
  void writeRelease() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    mtx->writeRelease();
  }
  bool tryWriteAcquire() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    return mtx->tryWriteAcquire();
  }
  bool tryUpgrade() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    return mtx->tryUpgrade();
  }
  void downgrade() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    mtx->downGrade();
  }
  bool owned() {
    RwMutex* mtx = static_cast<RwMutex*>(lock);
    return mtx->writeOwned();
  }
};

struct RecursiveRwMutexWrapper : public RwLockWrapper {
  RecursiveRwMutexWrapper(ptr_t mtx)
    : RwLockWrapper(mtx, RwMutexType::RECURSIVE) {}
  void readAcquire() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    mtx->readAcquire();
  }
  void readRelease() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    mtx->readRelease();
  }
  bool tryReadAcquire() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    return mtx->tryReadAcquire();
  }
  void writeAcquire() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    mtx->writeAcquire();
  }
  void writeRelease() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    mtx->writeRelease();
  }
  bool tryWriteAcquire() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    return mtx->tryWriteAcquire();
  }
  bool tryUpgrade() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    return mtx->tryUpgrade();
  }
  void downgrade() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    mtx->downGrade();
  }
  bool owned() {
    RecursiveRwMutex* mtx = static_cast<RecursiveRwMutex*>(lock);
    return mtx->writeOwned();
  }
};

static SpinLock lockMapLock;
static map<ptr_t,RwLockWrapper*,less<ptr_t>,KernelAllocator<pair<ptr_t,RwLockWrapper*>>> lockMap;

#define LOG_LOCK(lock, file, line)  \
  DBG::outln(DBG::Basic, __FUNCTION__, ':', FmtHex((lock)), " @ ", (file), ':', (line));

static RwLockWrapper* getLockWrapper(ptr_t lock) {
  ScopedLock<> lo(lockMapLock);
  KASSERT1(lock, "null rwlock");
  KASSERT1(lockMap.count(lock), "unknown rwlock");
  return lockMap[lock];
}

void KOS_RWLockReadLock(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockReadLock:", FmtHex(lw), " @ ", file, ':', line);
  lw->readAcquire();
}

void KOS_RWLockReadUnLock(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockReadUnLock:", FmtHex(lw), " @ ", file, ':', line);
  lw->readRelease();
}

int KOS_RWLockTryReadLock(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockTryLock:", FmtHex(lw), " @ ", file, ':', line);
  return lw->tryReadAcquire() ? 1 : 0;
}

void KOS_RWLockWriteLock(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockWriteLock:", FmtHex(lw), " @ ", file, ':', line);
  lw->writeAcquire();
}

void KOS_RWLockWriteUnLock(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockWriteUnLock:", FmtHex(lw), " @ ", file, ':', line);
  lw->writeRelease();
}

int KOS_RWLockTryWriteLock(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockWriteTryLock:", FmtHex(lw), " @ ",
      file, ':', line);
  return lw->tryWriteAcquire() ? 1 : 0;
}

int KOS_RWLockTryUpgrade(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockTryUpgrade:", FmtHex(lw), " @ ",
      file, ':', line);
  return lw->tryUpgrade() ? 1 : 0;
}

void KOS_RWLockDowngrade(ptr_t lock, const char* file, int line) {
  RwLockWrapper* lw = getLockWrapper(lock);
  DBG::outln(DBG::Basic, "KOS_RWLockDowngrade:", FmtHex(lw), " @ ",
      file, ':', line);
  lw->downgrade();
}

int KOS_RWLockOwned(ptr_t lock) {
  RwLockWrapper* lw = getLockWrapper(lock);
  return lw->owned() ? 1 : 0;
}

// SHOULD MATCH with sys/sys/lock.h
#define LO_RECURSABLE 0x00080000

void KOS_RWLockInit(ptr_t lock, int flags) {
  KASSERT1(lock, "null rwlock");
  if (flags & LO_RECURSABLE) {
    RecursiveRwMutex* mtx = new RecursiveRwMutex;
    lockMapLock.acquire();
    KASSERTN(!lockMap.count(mtx), "KOS lock already exists for mutex:",
        FmtHex(mtx));
    lockMap[mtx] = new RecursiveRwMutexWrapper(mtx);
    lockMapLock.release();
  } else {
    RwMutex* mtx = new RwMutex;
    lockMapLock.acquire();
    KASSERTN(!lockMap.count(mtx), "KOS lock already exists for mutex:",
        FmtHex(mtx));
    lockMap[mtx] = new RwMutexWrapper(mtx);
    lockMapLock.release();
  }
}
