#ifndef _KOS_PRINTF_h_
#define _KOS_PRINTF_h_

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

EXTERNC void putchar(char ch);

#undef EXTERNC

#endif /* _KOS_PRINTF_h_ */
