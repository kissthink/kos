#ifndef _KOS_Timer_h_
#define _KOS_Timer_h_ 1

#include "kos/Basic.h"

EXTERNC int KOS_Ticks();
EXTERNC void KOS_UpdateVolTick(void *td);
EXTERNC int KOS_GetVolTick(void *td);

#endif /* _KOS_Timer_h_ */
