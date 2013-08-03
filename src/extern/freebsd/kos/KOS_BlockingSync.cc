#include "kos/BlockingSync.h"

// KOS
#include <cstdint>
#include "ipc/BlockingSync.h"
#include "ipc/SpinLock.h"
#include "util/Output.h"

extern "C" void KOS_Lock_Init(void** kos_lock, int spin, int recurse) {
  if (spin) {
    if (recurse) *kos_lock = new RecursiveSpinLock();
    else *kos_lock = new SpinLock();
  } else {
    if (recurse) *kos_lock = new RecursiveMutex();
    else *kos_lock = new Mutex();
  }
}

extern "C" void KOS_SpinLock_Lock(void* kos_lock) {
  SpinLock* lock = static_cast<SpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->acquire();
}

extern "C" void KOS_SpinLock_UnLock(void* kos_lock) {
  SpinLock* lock = static_cast<SpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->release();
}

extern "C" void KOS_RSpinLock_Lock(void* kos_lock) {
  RecursiveSpinLock* lock = static_cast<RecursiveSpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->acquire();
}

extern "C" void KOS_RSpinLock_UnLock(void* kos_lock) {
  RecursiveSpinLock* lock = static_cast<RecursiveSpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->release();
}

extern "C" void KOS_Mutex_Lock(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->acquire();
}

extern "C" int KOS_Mutex_TryLock(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryAcquire() ? 1 : 0;
}

extern "C" void KOS_Mutex_UnLock(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->release();
}

extern "C" void KOS_RMutex_Lock(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->acquire();
}

extern "C" int KOS_RMutex_TryLock(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryAcquire() ? 1 : 0;
}

extern "C" void KOS_RMutex_UnLock(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->release();
}

extern "C" void KOS_RwMutex_Init(void** kos_lock, int recurse) {
  // TODO recurse
  *kos_lock = new RwMutex();
}

extern "C" void KOS_RwMutex_RLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->readAcquire();
}

extern "C" void KOS_RwMutex_WLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->writeAcquire();
}

extern "C" void KOS_RwMutex_RUnLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->readRelease();
}

extern "C" void KOS_RwMutex_WUnLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->writeRelease();
}

extern "C" int KOS_RwMutex_RTryLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryReadAcquire() ? 1 : 0;
}

extern "C" int KOS_RwMutex_WTryLock(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryWriteAcquire() ? 1 : 0;
}

extern "C" int KOS_RwMutex_TryUpgrade(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryUpgrade() ? 1 : 0;
}

extern "C" void KOS_RwMutex_Downgrade(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->downGrade();
}
