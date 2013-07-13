#ifndef _C_Semaphore_h_
#define _C_Semaphore_h_

#ifdef __cplusplus
extern "C" {
#endif

  void SemaphoreP(void *semImpl);
  int SemaphorePInterruptible(void *semImpl);
  int SemaphorePKillable(void *semImpl);
  int SemaphoreTryP(void *semImpl);
  int SemaphorePTimeout(void *semImpl, long jiffies);
  void SemaphoreV(void *semImpl);

#ifdef __cplusplus
}
#endif

#endif /* _C_Semaphore_h_ */
