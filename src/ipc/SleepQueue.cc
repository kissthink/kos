#include "ipc/SleepQueue.h"
#include "kern/Thread.h"
#include "kern/Scheduler.h"
using namespace std;

typedef std::list<Thread*,KernelAllocator<Thread*>> ThreadList;

SpinLock SleepQueue::lk;
map<ptr_t,ThreadList,less<ptr_t>,KernelAllocator<ptr_t>> SleepQueue::channels;

void SleepQueue::sleep(ptr_t chan) {
  KASSERT1(chan, "channel is NULL");
  lk.acquire();
  channels[chan].push_back(Processor::getCurrThread());
  kernelScheduler.suspend(lk);
}

void SleepQueue::wakeupOne(ptr_t chan) {
  KASSERT1(chan, "channel is NULL");
  KASSERT1(channels.count(chan), "unknown channel");
  ScopedLock<> lo(lk);
  ThreadList& tl = channels[chan];
  Thread* t = tl.front();
  kernelScheduler.start(*t);
  tl.pop_front();
  if (tl.empty()) {
    channels.erase(chan);
  }
}

void SleepQueue::wakeup(ptr_t chan) {
  KASSERT1(chan, "channel is NULL");
  KASSERT1(channels.count(chan), "unknown channel");
  ScopedLock<> lo(lk);
  ThreadList& tl = channels[chan];
  while (!tl.empty()) {
    Thread* t = tl.front();
    kernelScheduler.start(*t);
    tl.pop_front();
  }
  channels.erase(chan);
}
