#ifndef _KOS_MALLOC_h_
#define _KOS_MALLOC_h_

#include "kos/Basic.h"

EXTERNC void *KOS_Malloc(unsigned long size);
EXTERNC void KOS_Free(void *addr);
EXTERNC unsigned long KOS_GetMemorySize(void *addr);
EXTERNC int KOS_GetUpTime();

#endif /* _KOS_MALLOC_h_ */
