/******************************************************************************
    Copyright 2012-2013 Martin Karsten

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
#ifndef _SyncQueues_h_
#define _SyncQueues_h_ 1

#include "util/Log.h"
#include "util/SpinLock.h"
#include "mach/Processor.h"
#include "kern/Kernel.h"
#include "kern/Thread.h"

template<typename Buffer>
class ProdConsQueue {
  volatile SpinLock lk;
  size_t unclaimedElements; // baton passing to guarantee FIFO access
  Buffer elementQueue;
  EmbeddedQueue<Thread> waitQueue;

  void suspend() {
    waitQueue.push(Processor::getCurrThread());
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
  typedef typename Buffer::Element Element;

  explicit ProdConsQueue(size_t N = 0) : unclaimedElements(0), elementQueue(N) {}

  void append(const Element& elem) {                 // blocking append
    ScopedLock<> lo(lk);
    if likely(unclaimedElements == elementQueue.max_size()) suspend();
    else if unlikely(!resume()) unclaimedElements += 1;
    else KASSERT(elementQueue.empty(), elementQueue.size());
    elementQueue.push(elem);
  }

  bool appendFromISR(const Element& elem) {          // non-blocking append
    ScopedLockISR<> lo(lk);
    if likely(unclaimedElements == elementQueue.max_size()) return false;
    else if unlikely(!resume()) unclaimedElements += 1;
    else KASSERT(elementQueue.empty(), elementQueue.size());
    elementQueue.push(elem);
    return true;
  }

  Element remove() {                                 // blocking remove
    ScopedLock<> lo(lk);
    if unlikely(unclaimedElements == 0) suspend();
    else if unlikely(!resume()) unclaimedElements -= 1;
    else KASSERT(elementQueue.full(), elementQueue.size());
    Element elem = elementQueue.front();
    elementQueue.pop();
    return elem;
  }
};

template<typename ClientBuffer> class HollywoodClient;
template<typename ClientBuffer> class HollywoodServer;

// don't call us, we'll call you
template <typename ClientBuffer, typename Server>
class HollywoodQueue : private ProdConsQueue<ClientBuffer> {
public:
  HollywoodQueue(size_t max) : ProdConsQueue<ClientBuffer>(max) {}
  void call( typename ClientBuffer::Element& client ) { ProdConsQueue<ClientBuffer>::append(client); }
  void callback( Server& server ) { ProdConsQueue<ClientBuffer>::remove()->callback(server); }
  void callback() { ProdConsQueue<ClientBuffer>::remove()->callback(*this); }
};

template<typename ClientBuffer>
class HollywoodClient {
public:
  virtual void callback( HollywoodQueue<ClientBuffer,HollywoodServer<ClientBuffer>>& server ) = 0;
};

template<typename ClientBuffer>
class HollywoodServer : public HollywoodQueue<ClientBuffer,HollywoodServer<ClientBuffer>> {};

#endif /* _SyncQueues_h_ */
