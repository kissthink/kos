#include "sys/param.h"
#include "sys/lock.h"
#include "sys/sx.h"
#include "kos/kos_blockingSync.h"
#include "kos/kos_kassert.h"

struct lock_class lock_class_sx = {
  .lc_name = "sx",
  .lc_flags = LC_SLEEPLOCK | LC_SLEEPABLE | LC_RECURSABLE | LC_UPGRADABLE,
#if 0
  .lc_assert = assert_sx,
  .lc_lock = lock_sx,
  .lc_unlock = unlock_sx,
#endif
};

/**
 * Possible flags
 *
 * SX_NOADAPTIVE - if kernel is compiled without NO_ADAPTIVE_SX, sx will spin instead of sleep.
 * SX_RECURSE - specifies sx is a recursive lock
 * SX_QUIET - tell system to not log operations done on the lock  (XXX ignored)
 * SX_NOWITNESS - causes witness(4) to ignore this lock  (XXX ignored)
 * SX_DUPOK - causes witness(4) to ignore duplicates of this lock   (XXX ignored)
 * SX_NOPROFILE - do not profile this lock  (XXX ignored)
 */
void sx_init_flags(struct sx *sx, const char *description, int opts) {
  BSD_KASSERT0((opts & ~(SX_QUIET | SX_RECURSE | SX_NOWITNESS | SX_DUPOK |
      SX_NOPROFILE | SX_NOADAPTIVE)) == 0);

  int flags;
  flags = LO_SLEEPABLE | LO_UPGRADABLE;
  if (opts & SX_RECURSE) flags |= LO_RECURSABLE;

  flags |= opts & SX_NOADAPTIVE;
  sx->sx_lock = SX_LOCK_UNLOCKED;

  lock_init(&sx->lock_object, &lock_class_sx, description, NULL, flags);
  kos_rw_mutex_init(&sx->kos_lock, opts & SX_RECURSE);
  sx->lock_object.kos_lock = sx->kos_lock;
  sx->lock_object.kos_lock_type = 1;
}

// TODO support interruptible sleep
int _sx_slock(struct sx *sx, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_slock() of destroyed sx");
  kos_rw_mutex_read_acquire(sx->kos_lock);
  return 0;
}

// TODO support interruptible sleep
int _sx_xlock(struct sx *sx, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_xlock() of destroyed sx");
  kos_rw_mutex_write_acquire(sx->kos_lock);
  return 0;
}

int _sx_try_slock(struct sx *sx, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_try_slock() of destroyed sx");
  return kos_rw_mutex_read_try_acquire(sx->kos_lock);
}

int _sx_try_xlock(struct sx *sx, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_try_xlock() of destroyed sx");
  return kos_rw_mutex_write_try_acquire(sx->kos_lock);
}

void _sx_sunlock(struct sx *sx, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_sunlock() of destroyed sx");
  kos_rw_mutex_read_release(sx->kos_lock);
}

void _sx_xunlock(struct sx *sx, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_xunlock() of destroyed sx");
  kos_rw_mutex_write_release(sx->kos_lock);
}

/**
 * Try to do a non-blocking upgrade from a shared lock to an exclusive lock.
 * This will only succeed if this thread holds a single shared lock.
 * Return 1 if the upgrade succeed, 0 otherwise.
 */
int _sx_try_upgrade(struct sx *sx, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_try_upgrade() of destroyed sx");
  return kos_rw_mutex_upgrade(sx->kos_lock);
}

/**
 * Downgrade an unrecursed exclusive lock into a single shared lock.
 */
void _sx_downgrade(struct sx *sx, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((sx->sx_lock != SX_LOCK_DESTROYED), "sx_downgrade() of destroyed sx");
  kos_rw_mutex_downgrade(sx->kos_lock);
}

void sx_destroy(struct sx *sx) {
  // TODO check if the lock is still held
  // TODO check if lock is still recursed
  sx->sx_lock = SX_LOCK_DESTROYED;
  lock_destroy(&sx->lock_object);
}
