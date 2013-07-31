#include "sys/param.h"
#include "sys/systm.h"
#include "sys/lock.h"
#include "sys/mutex.h"
#include "sys/condvar.h"
#include "kos/kos_condVar.h"
#include "kos/kos_kassert.h"

void cv_init(struct cv *cvp, const char *desc) {
  cvp->cv_description = desc;
  cvp->cv_waiters = 0;
  kos_cv_init(&cvp->kos_cv);
}

/**
 * Wait on a condition variable. The current thread is placed on the condition
 * variable's wait queue and suspended. A cv_signal or cv_broadcast on the same
 * condition variable will resume the thread. The mutex is released before
 * sleeping and will be held on return. It is recommended that the mutex be
 * held when cv_signal or cv_broadcast are called.
 */
void _cv_wait(struct cv *cvp, struct lock_object *lock) {
  kos_cv_wait(cvp->kos_cv, lock->kos_lock, lock->kos_lock_type, 0);
}

/**
 * Wait on a condition variable. This function differs from cv_wait by
 * not acquiring the mutex after condition variable was signaled.
 */
void _cv_wait_unlock(struct cv *cvp, struct lock_object *lock) {
  kos_cv_wait(cvp->kos_cv, lock->kos_lock, lock->kos_lock_type, 1);
}

/**
 * Wait on a condition variable, allowing interruption by signals. Return 0 if
 * the thread was resumed with cv_signal or cv_broadcast, EINTR or ERESTART if
 * a signal was caught. If ERESTART is returned the system call should be
 * restarted if possible.
 */
int _cv_wait_sig(struct cv *cvp, struct lock_object *lock) {
  // TODO implement when signal is supported
  kos_reboot();
  return 0;
}

/**
 * Wait on a condition variable for at most timeo/hz seconds. Returns 0 if the
 * process was resumed by cv_signal or cv_broadcast, EWOULDBLOCK if the timeout
 * expries.
 */
int _cv_timedwait(struct cv *cvp, struct lock_object *lock, int timo) {
  // TODO implement when signal is supported
  kos_reboot();
  return 0;
}

/**
 * Wait on a condition variable for at most timo/hz seconds, allowing
 * interruption by signals. Returns 0 if the thread was resumed by cv_signal
 * or cv_broadcast, EWOULDBLOCK if the timeout expires, and EINTR or ERESTART if
 * a signal was caught.
 */
int _cv_timedwait_sig(struct cv *cvp, struct lock_object *lock, int timo) {
  // TODO implement when signal is supported
  kos_reboot();
  return 0;
}

/**
 * Signal a condition variable, wakes up one waiting thread. Will also wakeup
 * the swapper if the process is not in memory, so that it can bring the
 * sleeping process in. Note that this may also result in additional threads
 * being made runnable. Should be called with the same mutex as was passed to
 * cv_wait held.
 */
void cv_signal(struct cv *cvp) {
  kos_cv_signal(cvp->kos_cv);
}

/**
 * Broadcast a signal to a condition variable. Wakes up all waiting threads.
 * Should be called with the same mutex as was passed to cv_wait held.
 */
void cv_broadcastpri(struct cv *cvp, int pri) {
  // TODO respect priority
  kos_cv_broadcast(cvp->kos_cv);
}

/**
 * Destroy a condition variable. The condition variable must be re-initialized
 * in order to be re-used.
 */
void cv_destroy(struct cv *cvp) {
  kos_cv_destroy(cvp->kos_cv);
}
