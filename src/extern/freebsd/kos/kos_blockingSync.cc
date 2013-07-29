#include <cstdint>
#include "ipc/BlockingSync.h"
#include "ipc/SpinLock.h"
#include "util/Output.h"

void kos_lock_init(void** kos_lock, int spin, int recurse) {
  if (spin) {
    if (recurse) *kos_lock = new RecursiveSpinLock();
    else *kos_lock = new SpinLock();
  } else {
    if (recurse) *kos_lock = new RecursiveMutex();
    else *kos_lock = new Mutex();
  }
}

void kos_spinlock_acquire(void* kos_lock) {
  SpinLock* lock = static_cast<SpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->acquire();
}

void kos_spinlock_release(void* kos_lock) {
  SpinLock* lock = static_cast<SpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->release();
}

void kos_rspinlock_acquire(void* kos_lock) {
  RecursiveSpinLock* lock = static_cast<RecursiveSpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->acquire();
}

void kos_rspinlock_release(void* kos_lock) {
  RecursiveSpinLock* lock = static_cast<RecursiveSpinLock*>(kos_lock);
  KASSERT0(lock);
  lock->release();
}

void kos_mutex_acquire(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->acquire();
}

int kos_mutex_try_acquire(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryAcquire() ? 1 : 0;
}

void kos_mutex_release(void* kos_lock) {
  Mutex* mtx = static_cast<Mutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->release();
}

void kos_rmutex_acquire(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->acquire();
}

int kos_rmutex_try_acquire(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryAcquire() ? 1 : 0;
}

void kos_rmutex_release(void* kos_lock) {
  RecursiveMutex* mtx = static_cast<RecursiveMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->release();
}

void kos_rw_mutex_init(void** kos_lock, int recurse) {
  // TODO recurse
  *kos_lock = new RwMutex();
}

void kos_rw_mutex_read_acquire(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->readAcquire();
}

void kos_rw_mutex_write_acquire(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->writeAcquire();
}

void kos_rw_mutex_read_release(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->readRelease();
}

void kos_rw_mutex_write_release(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->writeRelease();
}

int kos_rw_mutex_read_try_acquire(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryReadAcquire() ? 1 : 0;
}

int kos_rw_mutex_write_try_acquire(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryWriteAcquire() ? 1 : 0;
}

int kos_rw_mutex_upgrade(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  return mtx->tryUpgrade() ? 1 : 0;
}

void kos_rw_mutex_downgrade(void* kos_lock) {
  RwMutex* mtx = static_cast<RwMutex*>(kos_lock);
  KASSERT0(mtx);
  mtx->downGrade();
}
