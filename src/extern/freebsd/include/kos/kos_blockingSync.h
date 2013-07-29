#ifndef _KOS_BLOCKINGSYNC_h_
#define _KOS_BLOCKINGSYNC_h_

/**
 * useful macros
 */

#if defined(EXTERNC)
#error macro collision: EXTERNC
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void kos_lock_init(void** kos_lock, int spin, int recurse);

// spinlock
EXTERNC void kos_spinlock_acquire(void* kos_lock);
EXTERNC void kos_spinlock_release(void* kos_lock);
EXTERNC void kos_rspinlock_acquire(void* kos_lock);
EXTERNC void kos_rspinlock_release(void* kos_lock);

// mutex
EXTERNC void kos_mutex_acquire(void* kos_lock);
EXTERNC int kos_mutex_try_acquire(void* kos_lock);
EXTERNC void kos_mutex_release(void* kos_lock);
EXTERNC void kos_rmutex_acquire(void* kos_lock);
EXTERNC int kos_rmutex_try_acquire(void* kos_lock);
EXTERNC void kos_rmutex_release(void* kos_lock);

// rw mutex
EXTERNC void kos_rw_mutex_init(void** kos_lock, int recurse);
EXTERNC void kos_rw_mutex_read_acquire(void* kos_lock);
EXTERNC void kos_rw_mutex_write_acquire(void* kos_lock);
EXTERNC void kos_rw_mutex_read_release(void* kos_lock);
EXTERNC void kos_rw_mutex_write_release(void* kos_lock);
EXTERNC int kos_rw_mutex_read_try_acquire(void* kos_lock);
EXTERNC int kos_rw_mutex_write_try_acquire(void* kos_lock);
EXTERNC int kos_rw_mutex_upgrade(void* kos_lock);
EXTERNC void kos_rw_mutex_downgrade(void* kos_lock);

#undef EXTERNC

#endif /* _KOS_BLOCKINGSYNC_h_ */
