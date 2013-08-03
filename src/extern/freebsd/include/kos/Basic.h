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

#endif /* _KOS_BASIC_h_ */
