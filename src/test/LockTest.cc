#include "kern/Debug.h"
#include "kern/Kernel.h"
#include "ipc/BlockingSync.h"
#include "ipc/CondVar.h"
#include "ipc/ReadWriteLock.h"
#include "mach/Processor.h"
#include "test/LockTest.h"
#include "util/Output.h"
#include <atomic>

ConditionVar cond;
Mutex mtx;
atomic<int> acquireCount;
atomic<int> releaseCount;
atomic<int> numThreadsDone;
Thread* testThread = nullptr;

// MUTEX TEST
static void mutex(ptr_t) {
  for (int i = 0; i < 10000; i++) {
    mtx.acquire();
    acquireCount += 1;
    mtx.release();
    releaseCount += 1;
  }
  DBG::outln(DBG::Basic, "thread ", Processor::getCurrThread()->getName(), " done");
  numThreadsDone += 1;
}

void MutexTest() {
  DBG::outln(DBG::Basic, "running MutexTest...");
  acquireCount = releaseCount = numThreadsDone = 0;
  Thread::create(mutex, nullptr, kernelSpace, "m1");
  Thread::create(mutex, nullptr, kernelSpace, "m2");
  Thread::create(mutex, nullptr, kernelSpace, "m3");
  Thread::create(mutex, nullptr, kernelSpace, "m4");
  Thread::create(mutex, nullptr, kernelSpace, "m5");
  Thread::create(mutex, nullptr, kernelSpace, "m6");
  Thread::create(mutex, nullptr, kernelSpace, "m7");
  Thread::create(mutex, nullptr, kernelSpace, "m8");
  Thread::create(mutex, nullptr, kernelSpace, "m9");
  Thread::create(mutex, nullptr, kernelSpace, "m10");

  while (numThreadsDone != 10) Pause();
  KASSERT1(acquireCount == releaseCount, "acquire/release counts differ");
  KASSERT1(acquireCount == 100000, "wrong number of acquire/release");
  DBG::outln(DBG::Basic, "MutexTest success");
}

// RECURSIVE MUTEX TEST
RecursiveMutex rmtx;

static void rmutex(ptr_t) {
  for (int i = 0; i < 1000; i++) {
    for (int i = 0; i < 10; i++) {
      rmtx.acquire();
      acquireCount += 1;
    }
    for (int i = 0; i < 10; i++) {
      rmtx.release();
      releaseCount += 1;
    }
  }
  numThreadsDone += 1;
}

void RecursiveMutexTest() {
  DBG::outln(DBG::Basic, "running RecursiveMutexTest...");
  acquireCount = releaseCount = numThreadsDone = 0;
  Thread::create(rmutex, nullptr, kernelSpace, "rm1");
  Thread::create(rmutex, nullptr, kernelSpace, "rm2");
  Thread::create(rmutex, nullptr, kernelSpace, "rm3");
  Thread::create(rmutex, nullptr, kernelSpace, "rm4");
  Thread::create(rmutex, nullptr, kernelSpace, "rm5");
  Thread::create(rmutex, nullptr, kernelSpace, "rm6");
  Thread::create(rmutex, nullptr, kernelSpace, "rm7");
  Thread::create(rmutex, nullptr, kernelSpace, "rm8");
  Thread::create(rmutex, nullptr, kernelSpace, "rm9");
  Thread::create(rmutex, nullptr, kernelSpace, "rm10");

  while (numThreadsDone != 10) Pause();
  KASSERT1(acquireCount == releaseCount, "acquire/release count differ");
  KASSERT1(acquireCount == 100000, "wrong number of acquire/release");
  DBG::outln(DBG::Basic, "RecursiveMutexTest success");
}

// SEMAPHORE TEST
Semaphore sem(1);

static void semaphore(ptr_t) {
  for (int i = 0; i < 1000; i++) {
    sem.P();
    acquireCount += 1;
    for (int i = 0; i < 1000; i++) {}
    sem.V();
    releaseCount += 1;
  }
  numThreadsDone += 1;
}

void SemaphoreTest() {
  DBG::outln(DBG::Basic, "running SemaphoreTest...");
  acquireCount = releaseCount = numThreadsDone = 0;
  Thread::create(semaphore, nullptr, kernelSpace, "s1");
  Thread::create(semaphore, nullptr, kernelSpace, "s2");
  Thread::create(semaphore, nullptr, kernelSpace, "s3");
  Thread::create(semaphore, nullptr, kernelSpace, "s4");
  Thread::create(semaphore, nullptr, kernelSpace, "s5");
  Thread::create(semaphore, nullptr, kernelSpace, "s6");
  Thread::create(semaphore, nullptr, kernelSpace, "s7");
  Thread::create(semaphore, nullptr, kernelSpace, "s8");
  Thread::create(semaphore, nullptr, kernelSpace, "s9");
  Thread::create(semaphore, nullptr, kernelSpace, "s10");

  while (numThreadsDone != 10) Pause();
  KASSERT1(acquireCount == releaseCount, "acquire/release count differ");
  KASSERT1(acquireCount == 10000, "wrong number of acquire/release");
  DBG::outln(DBG::Basic, "SemaphoreTest success");
}

// CONDITION VARIABLE TEST
static int counter = 0;
static atomic<int> threadsStarted;

static void waiter(ptr_t) {
  threadsStarted += 1;
  mtx.acquire();
  while (counter < 100000)
    cond.wait(&mtx);
  KASSERT1(counter == 100000, counter);
  mtx.release();
  numThreadsDone += 1;
}

static void signaller(ptr_t) {
  while (counter < 100000) counter++;
  cond.broadcast();
#if 0
  for (int i = 0; i < 100; i++) {
    cond.signal(&mtx);
    mtx.acquire();
  }
  mtx.release();
#endif
}

void ConditionVarTest() {
  DBG::outln(DBG::Basic, "running CVTest...");
  threadsStarted = numThreadsDone = 0;
  for (int i = 0; i < 200; i++) {
    Thread::create(waiter, nullptr, kernelSpace, "waiter");
  }
  while (threadsStarted != 200) Pause();
  Processor::getCurrThread()->sleep(5000);
  DBG::outln(DBG::Basic, "all waiters started");
  Thread::create(signaller, nullptr, kernelSpace, "signaller");
  while (numThreadsDone != 200) Pause();
  DBG::outln(DBG::Basic, "CVTest success");
}



RwMutex rwmtx;
atomic<int> readAcquireCount, readReleaseCount;
atomic<int> writeAcquireCount, writeReleaseCount;

static void rwmutex_readers(ptr_t) {
  for (int i = 0; i < 1000; i++) {
    rwmtx.readAcquire();
    readAcquireCount += 1;
    rwmtx.readRelease();
    readReleaseCount += 1;
    for (int i = 0; i < 10000; i++) {}
  }
  numThreadsDone += 1;
  DBG::outln(DBG::Basic, "thread ", Processor::getCurrThread()->getName(), " done");
}
static void rwmutex_writers(ptr_t) {
  for (int i = 0; i < 1000; i++) {
    rwmtx.writeAcquire();
    writeAcquireCount += 1;
    counter += 1;
    rwmtx.writeRelease();
    writeReleaseCount += 1;
  }
  numThreadsDone += 1;
  DBG::outln(DBG::Basic, "thread ", Processor::getCurrThread()->getName(), " done");
}

void RwMutexTest() {
  DBG::outln(DBG::Basic, "running RwMutexTest...");
  numThreadsDone = counter = 0;
  Thread::create(rwmutex_readers, nullptr, kernelSpace, "r1");
  Thread::create(rwmutex_readers, nullptr, kernelSpace, "r2");
  Thread::create(rwmutex_readers, nullptr, kernelSpace, "r3");
  Thread::create(rwmutex_readers, nullptr, kernelSpace, "r4");
  Thread::create(rwmutex_readers, nullptr, kernelSpace, "r5");

  Thread::create(rwmutex_writers, nullptr, kernelSpace, "w1");
  Thread::create(rwmutex_writers, nullptr, kernelSpace, "w2");
  Thread::create(rwmutex_writers, nullptr, kernelSpace, "w3");
  Thread::create(rwmutex_writers, nullptr, kernelSpace, "w4");
  Thread::create(rwmutex_writers, nullptr, kernelSpace, "w5");

  while (numThreadsDone != 10) Pause();
  KASSERT1(readAcquireCount == readReleaseCount, "read acquire/release counts differ");
  KASSERT1(readAcquireCount == 5000, "wrong read acquire/release value");
  KASSERT1(writeAcquireCount == writeReleaseCount, "write acquire/release counts differ");
  KASSERT1(writeAcquireCount == 5000, "wrong write acquire/release value");
  KASSERT1(counter == 5000, counter);
  DBG::outln(DBG::Basic, "RwMutexTest success");
}
