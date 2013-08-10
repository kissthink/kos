#include "sys/param.h"
#include "sys/lock.h"
#include "sys/mutex.h"
#include "kos/BlockingSync.h"
#include "kos/Kassert.h"

#if 0
static void assert_mtx(struct lock_object *lock, int what);
static void lock_mtx(struct lock_object *lock, int how);
static void lock_spin(struct lock_object *lock, int how);
static int unlock_mtx(struct lock_object *lock);
static int unlock_spin(struct lock_object *lock);
#endif

/**
 * Lock classes for sleep and spin mutexes.
 */
struct lock_class lock_class_mtx_sleep = {
  .lc_name = "sleep mutex",
  .lc_flags = LC_SLEEPLOCK | LC_RECURSABLE,
#if 0
  .lc_assert = assert_mtx,
  .lc_lock = lock_mtx,
  .lc_unlock = unlock_mtx,
#endif
};
struct lock_class lock_class_mtx_spin = {
  .lc_name = "spin mutex",
  .lc_flags = LC_SPINLOCK | LC_RECURSABLE,
#if 0
  .lc_assert = assert_mtx,
  .lc_lock = lock_spin,
  .lc_unlock = unlock_spin,
#endif
};

#if 0
/**
 * System-wide mutexes
 */
void assert_mtx(struct lock_object *lock, int what) {
  mtx_assert((struct mtx *)lock, what);
}
void lock_mtx(struct lock_object *lock, int how) {
  mtx_lock((struct mtx *)lock);
}
void lock_spin(struct lock_object *lock, int how) {
  BSD_KASSERTSTR(true, "spin locks can only use msleep_spin");
}
int unlock_mtx(struct lock_object *lock) {
  struct mtx *m;
  m = (struct mtx *)lock;
  mtx_assert(m, MA_OWNED | MA_NOTRECURSED);
  mtx_unlock(m);
  return 0;
}
#endif

/**
 * Possible options
 *
 * MTX_DEF - sleep mutex
 * MTX_SPIN - spin lock
 * MTX_RECURSE - recursive lock
 * MTX_QUIET - do not log lock operations (XXX ignored)
 * MTX_NOWITNESS - cause witness(4) to ignore lock (XXX ignored)
 * MTX_DUPOK - cause witness(4) to ignore duplicate of this lock (XXX ignored)
 * MTX_NOPROFILE - do not profile this lock (XXX ignored)
 */
void mtx_init(struct mtx *m, const char *name, const char *type, int opts) {
  // check if invalid options were given
  BSD_KASSERT0((opts & ~(MTX_SPIN | MTX_QUIET | MTX_RECURSE |
                MTX_NOWITNESS | MTX_DUPOK | MTX_NOPROFILE)) == 0);

  struct lock_class *class;
  int flags = 0;
  m->kos_lock = 0;
  m->mtx_lock = MTX_UNOWNED;
  if (opts & MTX_SPIN) class = &lock_class_mtx_spin;
  else class = &lock_class_mtx_sleep;
  if (opts & MTX_RECURSE) flags |= LO_RECURSABLE;

  lock_init(&m->lock_object, class, name, type, flags);     // init for FreeBSD data
  KOS_Lock_Init(&m->kos_lock, opts & MTX_SPIN, opts & MTX_RECURSE);
  m->lock_object.kos_lock = m->kos_lock;
  if (opts & MTX_SPIN) m->lock_object.kos_lock_type = 3;
  else m->lock_object.kos_lock_type = 0;
}

void _mtx_lock_flags(struct mtx *m, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((m->mtx_lock != MTX_DESTROYED), "mtx_lock() of destroyed mutex");
  BSD_KASSERTSTR((LOCK_CLASS(&m->lock_object) == &lock_class_mtx_sleep),
      "mtx_lock() of spin mutex");
  if (m->lock_object.lo_flags & LO_RECURSABLE) KOS_RMutex_Lock(m->kos_lock);
  else KOS_Mutex_Lock(m->kos_lock);
}

void _mtx_unlock_flags(struct mtx *m, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((m->mtx_lock != MTX_DESTROYED), "mtx_unlock() of destroyed mutex");
  BSD_KASSERTSTR((LOCK_CLASS(&m->lock_object) == &lock_class_mtx_sleep),
      "mtx_unlock() of spin mutex");
  if (m->lock_object.lo_flags & LO_RECURSABLE) KOS_RMutex_UnLock(m->kos_lock);
  else KOS_Mutex_UnLock(m->kos_lock);
}

void _mtx_lock_spin_flags(struct mtx *m, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((m->mtx_lock != MTX_DESTROYED), "mtx_lock_spin() of destroyed mutex");
  BSD_KASSERTSTR((LOCK_CLASS(&m->lock_object) == &lock_class_mtx_spin),
      "mtx_lock_spin() of sleep mutex");
  // XXX KOS does not handle acquiring spinlock recursively on non-recursive spinlock (i.e. deadlock)
  if (m->lock_object.lo_flags & LO_RECURSABLE) KOS_RSpinLock_Lock(m->kos_lock);
  else KOS_SpinLock_Lock(m->kos_lock);
}

void _mtx_unlock_spin_flags(struct mtx *m, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((m->mtx_lock != MTX_DESTROYED), "mtx_unlock_spin() of destroyed mutex");
  BSD_KASSERTSTR((LOCK_CLASS(&m->lock_object) == &lock_class_mtx_spin),
      "mtx_unlock_spin() of sleep mutex");
  if (m->lock_object.lo_flags & LO_RECURSABLE) KOS_RSpinLock_UnLock(m->kos_lock);
  else KOS_SpinLock_UnLock(m->kos_lock);
}

int _mtx_trylock(struct mtx *m, int opts, const char *file, int line) {
  // TODO return if kernel already panic'ed
  BSD_KASSERTSTR((m->mtx_lock != MTX_DESTROYED), "mtx_trylock() of destroyed mutex");
  BSD_KASSERTSTR((LOCK_CLASS(&m->lock_object) == &lock_class_mtx_sleep),
      "mtx_trylock() of spin mutex");
  if (m->lock_object.lo_flags & LO_RECURSABLE) return KOS_RMutex_TryLock(m->kos_lock);
  return KOS_Mutex_TryLock(m->kos_lock);
}

/**
 * Remove lock 'm' from all_mtx queue.
 */
void mtx_destroy(struct mtx *m) {
  // TODO maybe add a check to KOS that tells if lock is locked  
  m->mtx_lock = MTX_DESTROYED;
  lock_destroy(&m->lock_object);
}

/*
 * General init routine used by the MTX_SYSINIT() macro.
 */
void
mtx_sysinit(void *arg) {
  struct mtx_args *margs = arg;

  mtx_init(margs->ma_mtx, margs->ma_desc, NULL, margs->ma_opts);
}
