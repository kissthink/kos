#ifndef _KOS_Thread_h_
#define _KOS_Thread_h_ 1

#include "kos/Basic.h"

// creates a kernel thread and returns the created thread
EXTERNC void KOS_ThreadLock(void *thread, const char *, int);
EXTERNC void KOS_ThreadUnLock(void *thread, const char *, int);
EXTERNC int KOS_AWAITING_INTR(void *thread, int flag);
EXTERNC void KOS_TD_SET_IWAIT(void *thread, int flag);
EXTERNC void KOS_TD_CLR_IWAIT(void *thread, int flag);
EXTERNC void *KOS_CurThread();
EXTERNC void KOS_SetThreadName(void *thread, char *name);
EXTERNC void *KOS_ThreadsMalloc(int count);
EXTERNC int KOS_GetCpuId();
EXTERNC void *KOS_ThreadCreate(void (*func)(void *), void *arg, int flags, const char *fmt, ...);
EXTERNC void KOS_Schedule(void *thread, int priority);
EXTERNC void KOS_ScheduleLocked(void *thread, int priority);
EXTERNC void KOS_Suspend(void *thread);
EXTERNC void KOS_Yield();
EXTERNC void critical_enter();
EXTERNC void critical_exit();

#endif /* _KOS_Thread_h_ */