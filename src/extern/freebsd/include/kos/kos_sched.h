#ifndef _KOS_SCHED_h_
#define _KOS_SCHED_h_

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

EXTERNC int kos_sleep(void *ident, void *lock, int lock_type, int pri, int timeout);
EXTERNC void kos_wakeup(void* chan);
EXTERNC void kos_wakeup_one(void* chan);

#undef EXTERNC

#endif /* _KOS_SCHED_h_ */
