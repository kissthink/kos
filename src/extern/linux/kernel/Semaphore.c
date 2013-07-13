#include "linux/semaphore.h"
#include "Semaphore.h"

void down(struct semaphore *sem) {
  SemaphoreP(sem->semImpl);
}

int down_interruptible(struct semaphore *sem) {
  return SemaphorePInterruptible(sem->semImpl);    // FIXME when signals is implemented
}

int down_killable(struct semaphore *sem) {
  return SemaphorePKillable(sem->semImpl);         // FIXME when signals is implemented
}

// returns 0 if semaphore has been acquired; 1 otherwise
int down_trylock(struct semaphore *sem) {
  return SemaphoreTryP(sem->semImpl) ? 0 : 1;
}

int down_timeout(struct semaphore *sem, long jiffies) {
  return SemaphorePTimeout(sem->semImpl, jiffies);  // FIXME when timer is implemented
}

void up(struct semaphore *sem) {
  SemaphoreV(sem->semImpl);
}
