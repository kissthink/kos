#ifndef _KOS_ThreadData_h_
#define _KOS_ThreadData_h_ 1

#include "ipc/SpinLock.h"
#include "util/basics.h"

// DO NOT include in FreeBSD code

class Thread;
struct ThreadData {
  Thread* thread;
  void (*func)(ptr_t);
  ptr_t arg;
  bool running;
  SpinLock lk;
  int td_inhibitors;
  int lock_count;
  bool savedInterrupt;

  ThreadData(Thread *t) : thread(t), func(nullptr), arg(nullptr), running(false),
      td_inhibitors(0), lock_count(0), savedInterrupt(false) {}
};

#endif /* _KOS_ThreadData_h_ */
