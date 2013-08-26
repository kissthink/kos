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
#ifndef _BlockingSync_h_
#define _BlockingSync_h_ 1

#include "extern/stl/mod_list"
#include "mach/Processor.h"
#include "kern/Kernel.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"
#include "ipc/SpinLock.h"

class BlockingSync {
  InPlaceList<Thread*> threadQueue;
protected:
  SpinLock lock;
  ~BlockingSync() {
    ScopedLock<> sl(lock);
    while (!threadQueue.empty()) resume();
  }
  void suspend() {
    threadQueue.push_back(Processor::getCurrThread());
    kernelScheduler.suspend(lock);
  }
  Thread* resume() {
    Thread* t = threadQueue.front();
    threadQueue.pop_front();
    kernelScheduler.start(*t);
    return t;
  }
  bool waiting() {
    return !threadQueue.empty();
  }
};

class Semaphore : public BlockingSync {
  mword counter;
public:
  Semaphore(mword c = 0) : counter(c) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(Semaphore)); }
  void P() {
    lock.acquire();
    if likely(counter < 1) {
      suspend();                           // releases lock, no re-acquire
    } else {
      counter -= 1;
      lock.release();
    }
  }
  bool tryP() {
    ScopedLock<> sl(lock);
    if likely(counter < 1) return false;
    counter -= 1;
    return true;
  }
  void V() {
    ScopedLock<> sl(lock);
    if likely(waiting()) resume();         // baton-passing through counter == 0
    else counter += 1;
  }
};

class Mutex : public BlockingSync {
protected:
  Thread* owner;
public:
  Mutex() : owner(nullptr) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(Mutex)); }
  void acquire() {
    lock.acquire();
    if (owner) {
      suspend();                           // releases lock, no re-acquire
    } else {
      owner = Processor::getCurrThread();
      lock.release();
    }
  }
  void release() {
    ScopedLock<> sl(lock);
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    if likely(waiting()) owner = resume(); // baton-passing through owner set
    else owner = nullptr;
  }
};

#endif /* _BlockingSync_h_ */
