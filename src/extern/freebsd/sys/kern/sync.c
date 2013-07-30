#include "sys/param.h"
#include "sys/lock.h"
#include "sys/systm.h"
#include "kos/kos_kassert.h"
#include "kos/kos_sched.h"

static int pause_wchan;

/**
 * General sleep call. Suspends the current thread until a wakeup is
 * performed on the specified identifier. The thread will then be made
 * runnable with the specified priority. Sleeps at most timo/hz seconds
 * (0 means no timeout). If pri includes PCATCH flag, signals are checked
 * before and after sleeping, else signals are not checked. Returns 0 if
 * awakened, EWOULDBLOCK if the timeout expires. If PCATCH is set and a
 * signal needs to be delivered, ERESTART is returned if the current system
 * call should be restarted if possible, and EINTR is returned if the system
 * call should be interrupted by the signal (return EINTR).
 *
 * The lock argument is unlocked before the caller is suspended, and
 * re-locked before _sleep() returns. If priority includes the PDROP
 * flag the lock is not re-locked before returning.
 */
int _sleep(void *ident, struct lock_object *lock, int priority,
           const char *wmesg, int timo) {
  // TODO return if kernel already panic'ed
  void* kos_lock = lock ? lock->kos_lock : NULL;
  int lock_type = lock ? lock->kos_lock_type : -1;
  return kos_sleep(ident, kos_lock, lock_type, priority, timo);
}
/**
 * Make all threads sleeping on the specified identifier runnable.
 */
void wakeup(void *ident) {
  kos_wakeup(ident);
}

/**
 * Make a thread sleeping on the specified identifier runnable.
 * May wake more than one thread if a target thread is currently
 * swapped out.
 */
void wakeup_one(void *ident) {
  kos_wakeup_one(ident);
}

/**
 * pause() delays the calling thread by the given number of system ticks.
 * During cold bootup, pause() uses the DELAY() function instead of
 * the tsleep() function to do the waiting. The "timo" argument must be
 * greater than or equal to zero. A "timo" value of zero is equivalent
 * to a "timo" value of one.
 */
int pause(const char *wmesg, int timo) {
  BSD_KASSERTSTR((timo >= 0), "pause: timo must be >= 0");
  if (timo < 1) timo = 1;
  return tsleep(&pause_wchan, 0, wmesg, timo);
}
