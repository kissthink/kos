#ifndef _KOS_CONDVAR_h_
#define _KOS_CONDVAR_h_

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

EXTERNC void kos_cv_init(void** cvp);
EXTERNC void kos_cv_wait(void* cv, void* mutex, int lockType, int unlock);
EXTERNC void kos_cv_signal(void* cv);
EXTERNC void kos_cv_broadcast(void* cv);
EXTERNC void kos_cv_destroy(void** cvp);

#undef EXTERNC

#endif /* _KOS_CONDVAR_h_ */
