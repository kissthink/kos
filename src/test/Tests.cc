#include "Tests.h"
#include "LockTest.h"
#include "TimerTest.h"
#include "SleepQueueTest.h"
#include "InterruptTest.h"

void RunTests() {
  MutexTest();
  RecursiveMutexTest();
  SemaphoreTest();
  CVTest();
  RwMutexTest();
  DelayTest();
  TimerTest();
  SleepQueueTest();
  InterruptTest();
}
