#ifndef _KOS_DYNAMICTIMER_h_
#define _KOS_DYNAMICTIMER_h_

#include "kos/Basic.h"

EXTERNC int KOS_Cancel_Callout(void (*func)(void *), void *arg, int time, int safe);
EXTERNC int KOS_Reset_Callout(void (*func)(void *), void *arg, int time,
                              int newTime, void (*newFunc)(void *), void *newArg);

#endif /* _KOS_DYNAMICTIMER_h_ */
