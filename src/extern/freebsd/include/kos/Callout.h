#ifndef _KOS_Callout_h_
#define _KOS_Callout_h_ 1

#include "kos/Basic.h"

EXTERNC void *KOS_GetPerThreadCallout(void *);
EXTERNC void KOS_SetPerThreadCallout(void *, void *);

#endif /* _KOS_Callout_h_ */
