#include "Tests.h"
#include "LockTest.h"
#include "TimerTest.h"

void RunTests() {
  MutexTest();
  RecursiveMutexTest();
  SemaphoreTest();
  ConditionVarTest();
  RwMutexTest();
  TimerTest();
}
