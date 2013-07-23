#include <cstdint>
#include <cerrno>
#include "extern/linux/kos/Semaphore.h"
#include "ipc/BlockingSync.h"

void* SemaphoreCreate() {
  return new Semaphore();
}
// uninterruptible sleep
void SemaphoreP(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();
}
// sleep can be interrupted by signal
// FIXME signal has to be supported
int SemaphorePInterruptible(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();
  return 0;
}
// sleep can be interrupted by a fatal signal
// FIXME signal has to be supported
int SemaphorePKillable(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->P();
  return 0;
}
// try acquire without waiting
// returns 0 if the semaphore has been acquired or 1 if it cannot be acquired!
int SemaphoreTryP(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  return ((Semaphore *)semImpl)->tryP() ? 0 : 1;
}
// acquire semaphore within a specified time
int SemaphorePTimeout(void *semImpl, long jiffies) {
  KASSERT0(semImpl != nullptr);
  return ((Semaphore *)semImpl)->Ptimeout(jiffies) ? 0 : -ETIME;
}

void SemaphoreV(void *semImpl) {
  KASSERT0(semImpl != nullptr);
  ((Semaphore *)semImpl)->V();
}
