#include "util/Output.h"
#include "mach/Processor.h"
#include "kern/Debug.h"
#include "kern/Kernel.h"
#include "ipc/BlockingSync.h"
#include "test/LockTest.h"
#include <atomic>

Mutex mtx;
atomic<int> acquireCount;
atomic<int> releaseCount;
atomic<int> numThreadsDone;

static const int testcount = 1000;

// MUTEX TEST
static void mutex(ptr_t) {
  for (int i = 0; i < testcount; i++) {
    mtx.acquire();
    acquireCount += 1;
    if (i % 100 == 0) {
      DBG::outln(DBG::Basic, "thread ", Processor::getCurrThread()->getName(), ' ', i);
    }
    for (int j = 0; j < testcount; j++);
    mtx.release();
    releaseCount += 1;
  }
  DBG::outln(DBG::Basic, "thread ", Processor::getCurrThread()->getName(), " done");
  numThreadsDone += 1;
}

void MutexTest() {
  DBG::outln(DBG::Basic, "running MutexTest...");
  acquireCount = releaseCount = numThreadsDone = 0;
  Thread* t = Thread::create(kernelSpace, "m1");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m2");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m3");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m4");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m5");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m6");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m7");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m8");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m9");
  kernelScheduler.run(*t, mutex, nullptr);
  t = Thread::create(kernelSpace, "m10");
  kernelScheduler.run(*t, mutex, nullptr);

  DBG::outln(DBG::Basic, "MutexTest: all threads running...");
  while (numThreadsDone != 10) Pause();
  KASSERT1(acquireCount == releaseCount, "acquire/release counts differ");
  KASSERT1(acquireCount == testcount * 10, "wrong number of acquire/release");
  DBG::outln(DBG::Basic, "MutexTest success");
}

// SEMAPHORE TEST
Semaphore sem(1);

static void semaphore(ptr_t) {
  for (int i = 0; i < testcount; i++) {
    sem.P();
    acquireCount += 1;
    if (i % 100 == 0) {
      DBG::outln(DBG::Basic, "thread ", Processor::getCurrThread()->getName(), ' ', i);
    }
    for (int j = 0; j < testcount; j++);
    sem.V();
    releaseCount += 1;
  }
  numThreadsDone += 1;
}

void SemaphoreTest() {
  DBG::outln(DBG::Basic, "running SemaphoreTest...");
  acquireCount = releaseCount = numThreadsDone = 0;
  Thread* t = Thread::create(kernelSpace, "s1");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s2");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s3");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s4");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s5");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s6");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s7");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s8");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s9");
  kernelScheduler.run(*t, semaphore, nullptr);
  t = Thread::create(kernelSpace, "s10");
  kernelScheduler.run(*t, semaphore, nullptr);

  DBG::outln(DBG::Basic, "SemaphoreTest: all threads running...");
  while (numThreadsDone != 10) Pause();
  KASSERT1(acquireCount == releaseCount, "acquire/release count differ");
  KASSERT1(acquireCount == testcount * 10, "wrong number of acquire/release");
  DBG::outln(DBG::Basic, "SemaphoreTest success");
}