#include "extern/linux/kos/Semaphore.h"
#include "kern/BlockingSync.h"

void SemaphoreP(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();
}

int SemaphorePInterruptible(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();    // FIXME when signals is implemented
  return 0;
}

int SemaphorePKillable(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();
  return 0;
}

int SemaphoreTryP(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  return ((Semaphore *)semImpl)->tryP();
}

int SemaphorePTimeout(void *semImpl, long jiffies) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();
  return 0;
}

void SemaphoreV(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->V();
}
