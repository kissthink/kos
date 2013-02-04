/******************************************************************************
    Copyright 2012 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef _PCQ_h_
#define _PCQ_h_ 1

#include "util/Log.h"
#include "util/SpinLock.h"
#include "mach/Processor.h"
#include "kern/Kernel.h"
#include "kern/KernelHeap.h"
#include "kern/Thread.h"

#include <queue>

template<typename Element>
class PCQ {
  volatile SpinLock lk;
  size_t elemMax;
  size_t elemCount;
  KernelQueue<Element> elementQueue;
  EmbeddedQueue<Thread> waitQueue;

  void suspend() {
    waitQueue.push(Processor::getThread());
    kernelScheduler.suspend(lk);
    lk.acquire();
  }

  bool resume() {
    if unlikely(waitQueue.empty()) return false;
    kernelScheduler.start(*waitQueue.front());
    waitQueue.pop();
    return true;
  }

public:
  explicit PCQ(size_t max) : elemMax(max), elemCount(0) {}

  void append(const Element& elem) {
    LockObject<SpinLock> lo(lk);
    if likely(elemCount == elemMax) suspend();
    else if unlikely(!resume()) elemCount += 1;
    else KASSERT(elementQueue.size() == 0, elementQueue.size());
    elementQueue.push(elem);
  }

  bool appendISR(const Element& elem) {
    LockObjectISR<SpinLock> lo(lk);
    if likely(elemCount == elemMax) return false;
    else if unlikely(!resume()) elemCount += 1;
    else KASSERT(elementQueue.size() == 0, elementQueue.size());
    elementQueue.push(elem);
    return true;
  }

  Element remove() {
    LockObject<SpinLock> lo(lk);
    if unlikely(elemCount == 0) suspend();
    else if unlikely(!resume()) elemCount -= 1;
    else KASSERT(elementQueue.size() == elemMax, elementQueue.size());
    Element elem = elementQueue.front();
    elementQueue.pop();
    return elem;
  }
};

class AsyncPCQ; // clients supply callbacks, rather than elements

class DynamicPCQ; // dynamic combination of PCQs & PCQ-Connectors

#endif /* _PCQ_h_ */
