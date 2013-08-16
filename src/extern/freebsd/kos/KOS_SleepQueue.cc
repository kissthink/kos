#include "kos/SleepQueue.h"
#include "kos/ThreadData.h"

// KOS
#include <cstdint>
#include "ipc/RecursiveSpinLock.h"
#include "kern/Kernel.h"
#include "mach/Machine.h"
#include "util/Output.h"
#include <map>
#include <list>
using namespace std;

struct SleepQueue {
  ptr_t wchan;
  list<ThreadData*,KernelAllocator<ThreadData*>> blocked;

  SleepQueue(ptr_t wchan) : wchan(wchan) {}
};

static RecursiveSpinLock sqLock;  // global lock for sleep queue
static map<ptr_t,SleepQueue*,less<ptr_t>,KernelAllocator<pair<ptr_t,SleepQueue*>>> sqMap;

void KOS_SleepQueueLockAssert() {
  KASSERT1(sqLock.isOwner(), "should be holding sleep queue lock");
}

void KOS_SleepQueueLock() {
  sqLock.acquire();
}

void KOS_SleepQueueUnLock() {
  sqLock.release();
}

ptr_t KOS_SleepQueueLookUpLocked(ptr_t wchan) {
  KOS_SleepQueueLockAssert();
  if (sqMap.count(wchan)) {
    return sqMap[wchan];
  }
  return nullptr;
}

ptr_t KOS_SleepQueueGetChannel(ptr_t td) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  ThreadData* threadData = (ThreadData *)td;
  return threadData->wchan;
}

int KOS_OnSleepQueue(ptr_t td) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  ThreadData* threadData = (ThreadData *)td;
  return (threadData->wchan) ? 1 : 0;
}

ptr_t KOS_SleepQueueCreateLocked(ptr_t wchan) {
  KOS_SleepQueueLockAssert();
  KASSERTN(sqMap.count(wchan) == 0, "sleepqueue for wchan:", FmtHex(wchan), " exists.");
  sqMap[wchan] = new SleepQueue(wchan);
  return sqMap[wchan];
}

void KOS_SleepQueueBlockThreadLocked(ptr_t td, ptr_t sq, const char* wmesg) {
  KOS_SleepQueueLockAssert();
  SleepQueue* sleepq = (SleepQueue *)sq;
  KASSERT1(sqMap[sleepq->wchan] == sleepq, "sleepqueue conflict");
  ((ThreadData *)td)->wchan = sleepq->wchan;
  ((ThreadData *)td)->wmesg = wmesg;
  sleepq->blocked.push_back((ThreadData *)td);
}

unsigned int KOS_SleepQueueBlockedThreadCount(ptr_t sq) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  SleepQueue* sleepq = (SleepQueue *)sq;
  KASSERT1(sqMap[sleepq->wchan] == sleepq, "sleepqueue conflict");
  return sleepq->blocked.size();
}

void KOS_SleepQueueWait(ptr_t wchan, ptr_t td) {
  sqLock.acquire();
  KASSERTN(sqMap.count(wchan), "sleepqueue for wchan ", FmtHex(wchan), " does not exists.");
  SleepQueue* sleepq = (SleepQueue *)sqMap[wchan];
  bool found = false;
  for (ThreadData* t : sleepq->blocked) {
    if (t == td) {
      found = true;
      break;
    }
  }
  KASSERTN(found, "thread td:", FmtHex(td), " is not waiting for wchan:", FmtHex(wchan));
  ThreadData* threadData = (ThreadData *)td;
  KASSERT1(threadData->thread == Processor::getCurrThread(),
      "cannot sleep as some other thread");
  ((ThreadData *)td)->sleeping = true;
  kernelScheduler.suspend(sqLock);
}

void KOS_SleepQueueWakeupOne(ptr_t sq) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  SleepQueue* sleepq = (SleepQueue *)sq;
  KASSERT1(sqMap[sleepq->wchan] == sleepq, "sleepqueue conflict");
  KASSERT1(!sleepq->blocked.empty(), "no threads are blocked");
  ThreadData* td = (ThreadData *)sleepq->blocked.front();
  td->wchan = nullptr;  // for KOS_OnSleepQueue
  td->sleeping = false;
  sleepq->blocked.pop_front();
  kernelScheduler.start(*td->thread);
}

void KOS_SleepQueueWakeup(ptr_t sq) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  SleepQueue* sleepq = (SleepQueue *)sq;
  KASSERT1(sqMap[sleepq->wchan] == sleepq, "sleepqueue conflict");
  KASSERT1(!sleepq->blocked.empty(), "no threads are blocked");
  while (!sleepq->blocked.empty()) {
    ThreadData* td = (ThreadData *)sleepq->blocked.front();
    td->wchan = nullptr;  // for KOS_OnSleepQueue
    td->sleeping = false;
    sleepq->blocked.pop_front();
    kernelScheduler.start(*td->thread);
  }
}

void KOS_SleepQueueWakeupThread(ptr_t td, ptr_t wchan) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  KASSERTN(sqMap.count(wchan), "sleepqueue for wchan:", FmtHex(wchan), " does not exists.");
  SleepQueue* sleepq = (SleepQueue *) sqMap[wchan];
  for (ThreadData* threadData : sleepq->blocked) {
    if (threadData == td) {
      threadData->wchan = nullptr;  // for KOS_OnSleepQueue
      threadData->sleeping = false;
      sleepq->blocked.remove(threadData);
      kernelScheduler.start(*threadData->thread);
      break;
    }
  }
}

int KOS_SleepQueueGetFlags(ptr_t td) {
  ThreadData* threadData = (ThreadData *)td;
  ScopedLock<> lo(threadData->lk);
  return threadData->flags;
}

void KOS_SleepQueueSetFlags(ptr_t td, int val) {
  ThreadData* threadData = (ThreadData *)td;
  ScopedLock<> lo(threadData->lk);
  threadData->flags |= val;
}

void KOS_SleepQueueResetFlags(ptr_t td, int val) {
  ThreadData* threadData = (ThreadData *)td;
  ScopedLock<> lo(threadData->lk);
  threadData->flags &= ~val;
}

void KOS_SleepQueueSetSleeping(ptr_t td, int val) {
  ThreadData* threadData = (ThreadData *)td;
  ScopedLock<> lo(threadData->lk);
  if (val) threadData->sleeping = true;
  else threadData->sleeping = false;
}

int KOS_SleepQueueIsSleeping(ptr_t td) {
  ThreadData* threadData = (ThreadData *)td;
  ScopedLock<> lo(threadData->lk);
  return threadData->sleeping ? 1 : 0;
}
