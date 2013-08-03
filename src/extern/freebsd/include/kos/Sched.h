#ifndef _KOS_SCHED_h_
#define _KOS_SCHED_h_

#include "kos/Basic.h"

EXTERNC int   KOS_Sleep(void *ident, void *lock, int lock_type, int pri, int timeout);
EXTERNC void  KOS_Wakeup(void* chan);
EXTERNC void  KOS_Wakeup_One(void* chan);
EXTERNC int   KOS_InInterrupt();

#endif /* _KOS_SCHED_h_ */
