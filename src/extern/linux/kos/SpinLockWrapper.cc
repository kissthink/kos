#include "extern/linux/kos/SpinLock.h"
#include "util/SpinLock.h"

void *SpinLockCreate() {
  return new SpinLock();
}

void SpinLockAcquire(void *lockImpl) {
  KASSERT0(lockImpl != nullptr);
  ((SpinLock *)lockImpl)->acquire();
}

int SpinLockTryAcquire(void *lockImpl) {
  KASSERT0(lockImpl != nullptr);
  return ((SpinLock *)lockImpl)->tryAcquire() ? 1 : 0;
}

void SpinLockRelease(void *lockImpl) {
  KASSERT0(lockImpl != nullptr);
  ((SpinLock *)lockImpl)->release();
}

void SpinLockAcquireISR(void *lockImpl) {
  KASSERT0(lockImpl != nullptr);   // FIXME change to atomic allocation later
  ((SpinLock *)lockImpl)->acquireISR();
}

void SpinLockReleaseISR(void *lockImpl) {
  KASSERT0(lockImpl != nullptr);
  ((SpinLock *)lockImpl)->releaseISR();
}
