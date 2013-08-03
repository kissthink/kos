#ifndef _KOS_CONDVAR_h_
#define _KOS_CONDVAR_h_

#include "kos/Basic.h"

EXTERNC void KOS_CV_Init(void** cvp);
EXTERNC void KOS_CV_Wait(void* cv, void* mutex, int lockType, int unlock);
EXTERNC void KOS_CV_Signal(void* cv);
EXTERNC void KOS_CV_BroadCast(void* cv);
EXTERNC void KOS_CV_Destroy(void** cvp);

#endif /* _KOS_CONDVAR_h_ */
