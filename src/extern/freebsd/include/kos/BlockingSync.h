#ifndef _KOS_BLOCKINGSYNC_h_
#define _KOS_BLOCKINGSYNC_h_

#include "kos/Basic.h"

EXTERNC void  KOS_Lock_Init(void** kos_lock, int spin, int recurse);

// spinlock
EXTERNC void  KOS_SpinLock_Lock(void* kos_lock);
EXTERNC void  KOS_SpinLock_UnLock(void* kos_lock);
EXTERNC void  KOS_RSpinLock_Lock(void* kos_lock);
EXTERNC void  KOS_RSpinLock_UnLock(void* kos_lock);

// mutex
EXTERNC void  KOS_Mutex_Lock(void* kos_lock);
EXTERNC int   KOS_Mutex_TryLock(void* kos_lock);
EXTERNC void  KOS_Mutex_UnLock(void* kos_lock);
EXTERNC void  KOS_RMutex_Lock(void* kos_lock);
EXTERNC int   KOS_RMutex_TryLock(void* kos_lock);
EXTERNC void  KOS_RMutex_UnLock(void* kos_lock);

// read/write mutex
EXTERNC void  KOS_RwMutex_Init(void** kos_lock, int recurse);
EXTERNC void  KOS_RwMutex_RLock(void* kos_lock);
EXTERNC void  KOS_RwMutex_WLock(void* kos_lock);
EXTERNC void  KOS_RwMutex_RUnLock(void* kos_lock);
EXTERNC void  KOS_RwMutex_WUnLock(void* kos_lock);
EXTERNC int   KOS_RwMutex_RTryLock(void* kos_lock);
EXTERNC int   KOS_RwMutex_WTryLock(void* kos_lock);
EXTERNC int   KOS_RwMutex_TryUpgrade(void* kos_lock);
EXTERNC void  KOS_RwMutex_Downgrade(void* kos_lock);

#endif /* _KOS_BLOCKINGSYNC_h_ */
