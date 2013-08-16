#include <cstdint>
#include "kos/Thread.h"
#include "kos/ThreadData.h"

// KOS
#include "kern/Debug.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"
#include "ipc/SpinLock.h"

#include <map>
using namespace std;

#undef __STRICT_ANSI__  // to get access to vsnprintf
#include <cstdio>

#define RFSTOPPED (1<<17)

SpinLock threadLock;
static map<Thread*,ThreadData*,less<Thread*>,KernelAllocator<pair<Thread*,ThreadData*>>> threadMap;
SpinLock threadDataSetLock;
static set<ThreadData*,less<ThreadData*>,KernelAllocator<ThreadData*>> threadDataSet;

#define THREAD_DATA_ASSERT(td)  \
  threadDataSetLock.acquire();  \
  KASSERT1(threadDataSet.find((td)) != threadDataSet.end(), "unknown thread data"); \
  threadDataSetLock.release()

static void ThreadDataAssert(ThreadData* td) {
  KASSERT1(threadDataSet.find(td) != threadDataSet.end(), "unknown thread data");
}

void KOS_ThreadLock(void* thread, const char* file, int line) {
  DBG::outln(DBG::Basic, "acquiring threadlock:", FmtHex(thread), " @ ", file, ':', line);
  ThreadData* td = (ThreadData *) thread;
  THREAD_DATA_ASSERT(td);
#if 0
  if (td->lock_count == 0)  // disable interrupts
    td->savedInterrupt = Processor::disableInterrupts();
  td->lock_count += 1;
#endif
  td->lk.acquire();
  DBG::outln(DBG::Basic, "acquired threadlock:", FmtHex(thread), " @ ", file, ':', line, " td_count:", td->lock_count);
}

void KOS_ThreadUnLock(void* thread, const char* file, int line) {
  DBG::outln(DBG::Basic, "releasing threadlock:", FmtHex(thread), " @ ", file, ':', line);
  ThreadData* td = (ThreadData *) thread;
  THREAD_DATA_ASSERT(td);
  td->lk.release();
#if 0
  td->lock_count -= 1;
  if (td->lock_count == 0) {
    if (td->savedInterrupt) { // restore interrupt
      td->savedInterrupt = false;
      Processor::enableInterrupts();
    }
  }
#endif
  DBG::outln(DBG::Basic, "released threadlock:", FmtHex(thread), " @ ", file, ':', line, " td_count:", td->lock_count);
}

// used in sys/kern/kern_intr.c
int KOS_AWAITING_INTR(void* thread, int flag) {
  ThreadData* td = (ThreadData *) thread;
  THREAD_DATA_ASSERT(td);
  return td->td_inhibitors & flag;
}
void KOS_TD_SET_IWAIT(void* thread, int flag) {
  ThreadData* td = (ThreadData *) thread;
  THREAD_DATA_ASSERT(td);
  td->td_inhibitors |= flag;
}
void KOS_TD_CLR_IWAIT(void* thread, int flag) {
  ThreadData* td = (ThreadData *) thread;
  THREAD_DATA_ASSERT(td);
  td->td_inhibitors &= ~flag;
}

ptr_t KOS_CurThread() {
  ScopedLock<> lo(threadLock);
  Thread* t = Processor::getCurrThread();
//  KASSERT1(threadMap.count(t), "thread is not in threadMap");
  if (!threadMap.count(t)) {
    threadMap[t] = new ThreadData(t);
  }
  return threadMap[t];
}

void KOS_SetThreadName(void* thread, char* name) {
  // TODO
}

ptr_t KOS_ThreadsMalloc(int count) {
  return new ThreadData*[count];
}

int KOS_GetCpuId() {
  return Processor::getApicID();
}

ptr_t KOS_ThreadCreate(void (*func)(ptr_t), ptr_t arg, int flags, const char* fmt, ...) {
  char name[20];
  va_list args;
  va_start(args, fmt);
  vsnprintf(name, 20, fmt, args);
  va_end(args);
  ThreadData* td;
  if (flags & RFSTOPPED) {
    Thread* t = Thread::create(kernelSpace, name);
    td = new ThreadData(t);
    threadLock.acquire();
    threadMap[t] = td;
    threadDataSetLock.acquire();
    auto result = threadDataSet.insert(td);
    KASSERT1(result.second, "thread data already exists");
    threadDataSetLock.release();
    threadLock.release();
    ScopedLock<> lo(td->lk);
    td->func = func;
    td->arg = arg;
    td->running = false;
  } else {
    Thread* t = Thread::create(func, arg, kernelSpace, name);
    td = new ThreadData(t);
    threadLock.acquire();
    threadMap[t] = td;
    threadDataSetLock.acquire();
    auto result = threadDataSet.insert(td);
    KASSERT1(result.second, "thread data already exists");
    threadDataSetLock.release();
    threadLock.release();
    ScopedLock<> lo(td->lk);
    td->func = func;
    td->arg = arg;
    td->running = true;
  }
  return td;
}

void KOS_Schedule(ptr_t thread, int priority) {
  ThreadData* td = (ThreadData*) thread;
  td->lk.acquire();
  if (td->running) {
    td->lk.release();
    kernelScheduler.start(*td->thread);
  } else {
    td->running = true;
    td->lk.release();
    td->thread->run(td->func, td->arg);
  }
}

void KOS_ScheduleLocked(ptr_t thread, int priority) {
  ThreadData* td = (ThreadData *) thread;
  if (td->running) {
    kernelScheduler.start(*td->thread);
  } else {
    td->running = true;
    td->thread->run(td->func, td->arg);
  }
}

void KOS_Suspend(ptr_t thread) {
  ThreadData* td = (ThreadData *) thread;
  bool locked = td->lk.tryAcquire();
  kernelScheduler.suspend(td->lk);
  if (!locked)    // locked before KOS_Suspend is called
    td->lk.acquire();
}

void KOS_Yield() {
  kernelScheduler.yield();
}

void critical_enter() {
  Processor::incLockCount();
}
void critical_exit() {
  Processor::decLockCount();
}
