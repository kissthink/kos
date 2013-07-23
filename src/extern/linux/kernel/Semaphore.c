#include "linux/semaphore.h"
#include "Semaphore.h"

void initSemaphore(struct semaphore* sem) {
  if (!__sync_val_compare_and_swap(&sem->semImplInit, 0, 1))
    sem->semImpl = SemaphoreCreate();
}
void down(struct semaphore *sem) {
  SemaphoreP(sem->semImpl);
}
int down_interruptible(struct semaphore *sem) {
  return SemaphorePInterruptible(sem->semImpl);    // FIXME when signals is implemented
}
int down_killable(struct semaphore *sem) {
  return SemaphorePKillable(sem->semImpl);         // FIXME when signals is implemented
}
int down_trylock(struct semaphore *sem) {
  return SemaphoreTryP(sem->semImpl);
}
int down_timeout(struct semaphore *sem, long jiffies) {
  return SemaphorePTimeout(sem->semImpl, jiffies);  // FIXME when timer is implemented
}
void up(struct semaphore *sem) {
  SemaphoreV(sem->semImpl);
}
