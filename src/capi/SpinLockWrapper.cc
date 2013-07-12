#include "capi/SpinLock.h"
#include "util/SpinLock.h"

// Initialize SpinLock if lockImpl is invalid
// Since initializing lockImpl from Linux's C functions
// is complicated, I decided to take lazy approach
void SpinLockAcquire(void **lockImpl) {
  SpinLock* lock;
  if (*lockImpl == nullptr) {
    lock = new SpinLock();
    *lockImpl = (void *)lock;
  }
  lock = (SpinLock *)(*lockImpl);
  lock->acquire();
}

int SpinLockTryAcquire(void **lockImpl) {
  SpinLock* lock;
  if (*lockImpl == nullptr) {
    lock = new SpinLock();
    *lockImpl = (void *)lock;
  }
  lock = (SpinLock *)(*lockImpl);
  return lock->tryAcquire() ? 1 : 0;
}

void SpinLockRelease(void **lockImpl) {
  KASSERT0(*lockImpl != nullptr);
  SpinLock* lock = (SpinLock *)(*lockImpl);
  lock->release();
}

void SpinLockAcquireISR(void **lockImpl) {
  KASSERT0(*lockImpl != nullptr);   // FIXME change to atomic allocation later
  SpinLock* lock = (SpinLock *)(*lockImpl);
  lock->acquireISR();
}

void SpinLockReleaseISR(void **lockImpl) {
  KASSERT0(*lockImpl != nullptr);
  SpinLock* lock = (SpinLock *)(*lockImpl);
  lock->releaseISR();
}
