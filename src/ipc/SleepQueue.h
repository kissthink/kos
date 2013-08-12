#ifndef _SleepQueue_h_
#define _SleepQueue_h_ 1

#include "kern/Kernel.h"
#include "ipc/SpinLock.h"
#include "mach/platform.h"

#include <list>
#include <map>

class Thread;

class SleepQueue {
  static SpinLock lk;
  static std::map<ptr_t,std::list<Thread*,KernelAllocator<Thread*>>,less<ptr_t>,KernelAllocator<ptr_t>> channels;

  SleepQueue() = delete;
  SleepQueue(const SleepQueue& sq) = delete;
  const SleepQueue& operator=(const SleepQueue&) = delete;
public:
  static void sleep(ptr_t chan);
  static void wakeupOne(ptr_t chan);
  static void wakeup(ptr_t chan);
};

#endif /* _SleepQueue_h_ */
