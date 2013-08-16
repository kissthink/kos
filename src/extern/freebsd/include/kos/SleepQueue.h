#ifndef _KOS_SleepQueue_h_
#define _KOS_SleepQueue_h_ 1

#include "kos/Basic.h"

EXTERNC void KOS_SleepQueueLockAssert();
EXTERNC void KOS_SleepQueueLock();
EXTERNC void KOS_SleepQueueUnLock();
EXTERNC void *KOS_SleepQueueLookUpLocked(void *wchan);
EXTERNC void *KOS_SleepQueueGetChannel(void *td);
EXTERNC int  KOS_OnSleepQueue(void *td);
EXTERNC void *KOS_SleepQueueCreateLocked(void *wchan, const char *wmesg);
EXTERNC void KOS_SleepQueueBlockThreadLocked(void *td, void *sq);
EXTERNC void KOS_SleepQueueResetTimeout(void *wchan, int timeout);
EXTERNC unsigned int KOS_SleepQueueBlockedThreadCount(void *sq);
EXTERNC void KOS_SleepQueueWait(void *wchan, void *td);
EXTERNC int KOS_SleepQueueCheckTimeout(void *wchan);
EXTERNC void KOS_SleepQueueWakeupOne(void *sq);
EXTERNC void KOS_SleepQueueWakeup(void *sq);
EXTERNC void KOS_SleepQueueWakeupThread(void *td, void *wchan);

#endif /* _KOS_SleepQueue_h_ */
