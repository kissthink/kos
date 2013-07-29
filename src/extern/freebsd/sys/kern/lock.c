#include "sys/param.h"
#include "sys/lock.h"
#include "kos/kos_kassert.h"

struct lock_class *lock_classes[LOCK_CLASS_MAX + 1] = {
  &lock_class_mtx_spin,
  &lock_class_mtx_sleep,
  /*
  &lock_class_sx,
  &lock_class_rm,
  &lock_class_rw,
  &lock_class_lockmgr,
  */
  NULL,
  NULL,
  NULL,
  NULL,
};

void lock_init(struct lock_object *lock, struct lock_class *class, const char *name,
               const char *type, int flags) {
  int i;
  for (i = 0; i < LOCK_CLASS_MAX; i++) {
    if (lock_classes[i] == class) {
      lock->lo_flags = i << LO_CLASSSHIFT;
      break;
    }
  }
  BSD_KASSERTSTR(i < LOCK_CLASS_MAX, "unkown lock class");

  // initialize lock object
  lock->lo_name = name;
  lock->lo_flags |= flags | LO_INITIALIZED;
}

void lock_destroy(struct lock_object *lock) {
  BSD_KASSERTSTR(lock_initalized(lock), "lock is not initialized");
  lock->lo_flags &= ~LO_INITIALIZED;
}
