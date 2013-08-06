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
