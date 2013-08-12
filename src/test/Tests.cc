#include "Tests.h"
#include "LockTest.h"
#include "TimerTest.h"
#include "SleepQueueTest.h"

void RunTests() {
  MutexTest();
  RecursiveMutexTest();
  SemaphoreTest();
  ConditionVarTest();
  RwMutexTest();
  DelayTest();
//  TimerTest();
  SleepQueueTest();
}
