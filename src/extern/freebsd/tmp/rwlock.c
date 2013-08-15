#include "sys/param.h"
#include "sys/lock.h"
#include "sys/mutex.h"
#include "sys/rwlock.h"
#include "kos/BlockingSync.h"
#include "kos/Kassert.h"

struct lock_class lock_class_rw = {
  .lc_name = "rw",
  .lc_flags = LC_SLEEPLOCK | LC_RECURSABLE | LC_UPGRADABLE,
#if 0
  .lc_assert = assert_rw,
  .lc_lock = lock_rw,
  .lc_unlock = unlock_rw,
#endif
};

/**
 * Possible flags
 *
 * RW_RECURSE - recursive lock
 * RW_QUIET - do not log lock operations
 * RW_NOWITNESS - causes witness(4) to ignore this lock
 * RW_DUPOK - causes witness(4) to ignore duplicates of this lock
 * RW_NOPROFILE - instructs the system to not profile this lock
 */
void rw_init_flags(struct rwlock *rw, const char *name, int opts) {
  int flags;

  BSD_KASSERT0((opts & ~(RW_DUPOK | RW_NOPROFILE | RW_NOWITNESS | RW_QUIET |
      RW_RECURSE)) == 0);

  flags = LO_UPGRADABLE;
  if (opts & RW_RECURSE) flags |= LO_RECURSABLE;

  rw->rw_lock = RW_UNLOCKED;
  lock_init(&rw->lock_object, &lock_class_rw, name, NULL, flags);
  KOS_RwMutex_Init(&rw->kos_lock, opts & RW_RECURSE);
  rw->lock_object.kos_lock = rw->kos_lock;
  rw->lock_object.kos_lock_type = 1;
}

void _rw_rlock(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_rlock() of destroyed rwlock");
  KOS_RwMutex_RLock(rw->kos_lock);
}

void _rw_wlock(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_wlock() of destroyed rwlock");
  KOS_RwMutex_WLock(rw->kos_lock);
}

int _rw_try_rlock(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_try_rlock() of destroyed rwlock");
  return KOS_RwMutex_RTryLock(rw->kos_lock);
}

int _rw_try_wlock(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_try_wlock() of destroyed rwlock");
  return KOS_RwMutex_WTryLock(rw->kos_lock);
}

void _rw_runlock(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_runlock() of destroyed rwlock");
  KOS_RwMutex_RUnLock(rw->kos_lock);
}

void _rw_wunlock(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_wunlock() of destroyed rwlock");
  KOS_RwMutex_WUnLock(rw->kos_lock);
}

int _rw_try_upgrade(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_try_upgrade() of destroyed rwlock");
  return KOS_RwMutex_TryUpgrade(rw->kos_lock);
}

void _rw_downgrade(struct rwlock *rw, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((rw->rw_lock != RW_DESTROYED), "rw_downgrade() of destroyed rwlock");
  KOS_RwMutex_Downgrade(rw->kos_lock);
}

void rw_destroy(struct rwlock *rw) {
  // TODO check if still locked
  // TODO check if in recurse
  rw->rw_lock = RW_DESTROYED;
  lock_destroy(&rw->lock_object);
}
