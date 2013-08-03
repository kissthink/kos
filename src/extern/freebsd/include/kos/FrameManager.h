#ifndef _KOS_FRAMEMANAGER_h_
#define _KOS_FRAMEMANAGER_h_

#include "kos/Basic.h"

#include <stdint.h>
typedef uint64_t vaddr;
EXTERNC void *KOS_Alloc_Contig(unsigned long size, vaddr low, vaddr high,
                               unsigned long alignment, unsigned long boundary);
EXTERNC void KOS_Free_Contig(void *addr, unsigned long size);

#endif /* _KOS_FRAMEMANAGER_h_ */
