#ifndef _C_SpinLock_h_
#define _C_SpinLock_h_

#ifdef __cplusplus
extern "C" {
#endif

  void *SpinLockCreate();
  void SpinLockAcquire(void *lockImpl);
  int SpinLockTryAcquire(void *lockImpl);
  void SpinLockRelease(void *lockImpl);
  void SpinLockAcquireISR(void *lockImpl);
  void SpinLockReleaseISR(void *lockImpl);

#ifdef __cplusplus
}
#endif

#endif /* _C_SpinLock_h_ */
