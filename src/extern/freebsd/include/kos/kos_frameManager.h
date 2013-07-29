#ifndef _KOS_FRAMEMANAGER_h_
#define _KOS_FRAMEMANAGER_h_

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

#include <stdint.h>
typedef uint64_t vaddr;
EXTERNC void *kos_alloc_contig(unsigned long size, vaddr low, vaddr high,
                               unsigned long alignment, unsigned long boundary);
EXTERNC void kos_free_contig(void *addr, unsigned long size);

#undef EXTERNC

#endif /* _KOS_FRAMEMANAGER_h_ */
