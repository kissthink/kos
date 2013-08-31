#ifndef _SimpleProdConsQueue_h_
#define _SimpleProdConsQueue_h_ 1

#include "kern/Kernel.h"
#include "ipc/BlockingSync.h"
#include "ipc/SpinLock.h"
#include <list>

template <typename Element>
class SimpleProdConsQueue {
  list<Element,KernelAllocator<Element>> queue;
  SpinLock lk;
  Semaphore avail;
  size_t curSize;
  size_t maxSize;
  InPlaceList<Thread*> waitQueue;

public:
  explicit SimpleProdConsQueue( size_t size ) : curSize(0), maxSize(size) {}
  bool tryAppend(const Element& elem) {
    lk.acquire();
    if (curSize == maxSize) {
      lk.release();
      return false;
    }
    queue.push_back(elem);
    curSize += 1;
    lk.release();
    avail.V();
    return true;
  }
  void append(const Element& elem) {
    lk.acquire();
    while (curSize == maxSize) {
      waitQueue.push_back(Processor::getCurrThread());
      kernelScheduler.suspend(lk);
      lk.acquire();
    }
    queue.push_back(elem);
    curSize += 1;
    lk.release();
    avail.V();
  }
  Element remove() {
    avail.P();
    ScopedLock<> lo(lk);
    Element elem = queue.front();
    queue.pop_front();
    curSize -= 1;
    if (!waitQueue.empty()) {
      Thread* t = waitQueue.front();
      waitQueue.pop_front();
      kernelScheduler.start(*t);
    }
    return elem;
  }
};

#endif /* _SimpleProdConsQueue_h_ */
