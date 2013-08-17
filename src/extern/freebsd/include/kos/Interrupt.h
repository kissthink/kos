#ifndef _KOS_Interrupt_h_
#define _KOS_Interrupt_h_ 1

#include "kos/Basic.h"

EXTERNC void KOS_IntrSetIWAIT(void *td);
EXTERNC void KOS_IntrResetIWAIT(void *td);
EXTERNC int KOS_IntrAwaitingIntr(void *td);

#endif /* _KOS_Interrupt_h_ */
