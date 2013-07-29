#ifndef _KOS_BASIC_h_
#define _KOS_BASIC_h_

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

#define likely(x)   (__builtin_expect((x),1))
#define unlikely(x) (__builtin_expect((x),0))

#endif /* _KOS_BASIC_h_ */
