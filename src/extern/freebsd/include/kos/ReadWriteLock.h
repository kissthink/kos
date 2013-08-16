#ifndef _KOS_ReadWriteLock_h_
#define _KOS_ReadWriteLock_h_ 1

#include "kos/Basic.h"

EXTERNC void KOS_RWLockInit(void *lock, int flags);
EXTERNC void KOS_RWLockReadLock(void *lock, const char *file, int line);
EXTERNC void KOS_RWLockReadUnLock(void *lock, const char *file, int line);
EXTERNC int KOS_RWLockTryReadLock(void *lock, const char *file, int line);
EXTERNC void KOS_RWLockWriteLock(void *lock, const char *file, int line);
EXTERNC void KOS_RWLockWriteUnLock(void *lock, const char *file, int line);
EXTERNC int KOS_RWLockTryWriteLock(void *lock, const char *file, int line);
EXTERNC int KOS_RWLockTryUpgrade(void *lock, const char *file, int line);
EXTERNC void KOS_RWLockDowngrade(void *lock, const char *file, int line);
EXTERNC int KOS_RWLockOwned(void *lock);

#endif /* _KOS_ReadWriteLock_h_ */
