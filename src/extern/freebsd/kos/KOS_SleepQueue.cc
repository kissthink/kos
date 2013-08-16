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
  const char* wmesg;
  bool timeout_checked;
  bool timedout;
  mword timeout;

  SleepQueue(ptr_t wchan, const char* wmesg) : wchan(wchan), wmesg(wmesg),
      timeout_checked(false), timedout(false), timeout(0) {}
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

ptr_t KOS_SleepQueueCreateLocked(ptr_t wchan, const char* wmesg) {
  KOS_SleepQueueLockAssert();
  KASSERTN(sqMap.count(wchan) == 0, "sleepqueue for wchan:", FmtHex(wchan), " exists.");
  sqMap[wchan] = new SleepQueue(wchan, wmesg);
  return sqMap[wchan];
}

void KOS_SleepQueueBlockThreadLocked(ptr_t td, ptr_t sq) {
  KOS_SleepQueueLockAssert();
  SleepQueue* sleepq = (SleepQueue *)sq;
  KASSERT1(sqMap[sleepq->wchan] == sleepq, "sleepqueue conflict");
  ((ThreadData *)td)->wchan = sleepq->wchan;
  sleepq->blocked.push_back((ThreadData *)td);
}

void KOS_SleepQueueResetTimeout(ptr_t wchan, int timeout) {
  // TODO: use callout
  KASSERT1(timeout >= 0, timeout);
  if (timeout == 0) {
    timeout = 1;
  }
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
  sleepq->timeout_checked = false;
  kernelScheduler.suspend(sqLock);
  sqLock.acquire();
  if (sleepq->timeout >= Machine::now() && !sleepq->timeout_checked) {
    sleepq->timedout = true;
  }
  if (sleepq->timeout_checked) {
    sleepq->timeout_checked = false;
    sleepq->timedout = false;
    sleepq->timeout = 0;
  }
  sqLock.release();
}

int KOS_SleepQueueCheckTimeout(ptr_t wchan) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  KASSERTN(sqMap.count(wchan), "sleepqueue for wchan:", FmtHex(wchan), " does not exists.");
  SleepQueue* sleepq = (SleepQueue *)sqMap[wchan];
  int timedout = sleepq->timedout;
  sleepq->timeout_checked = true;
  sleepq->timedout = false;
  return timedout ? 1 : 0;
}

void KOS_SleepQueueWakeupOne(ptr_t sq) {
  ScopedLock<RecursiveSpinLock> lo(sqLock);
  SleepQueue* sleepq = (SleepQueue *)sq;
  KASSERT1(sqMap[sleepq->wchan] == sleepq, "sleepqueue conflict");
  KASSERT1(!sleepq->blocked.empty(), "no threads are blocked");
  ThreadData* td = (ThreadData *)sleepq->blocked.front();
  td->wchan = nullptr;
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
    td->wchan = nullptr;
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
      threadData->wchan = nullptr;
      sleepq->blocked.remove(threadData);
      kernelScheduler.start(*threadData->thread);
      break;
    }
  }
}
