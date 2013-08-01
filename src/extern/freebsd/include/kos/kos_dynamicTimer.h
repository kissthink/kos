#ifndef _KOS_DYNAMICTIMER_h_
#define _KOS_DYNAMICTIMER_h_

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

EXTERNC int kos_cancel_callout(void (*func)(void *), void *arg, int time, int safe);

#undef EXTERNC

#endif /* _KOS_DYNAMICTIMER_h_ */
