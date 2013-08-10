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
  SpinLock lk;
  ~BlockingSync() {
    ScopedLock<> lo(lk);
    while (!threadQueue.empty()) resume();
  }
  void suspend() {
    threadQueue.push_back(Processor::getCurrThread());
    kernelScheduler.suspend(lk);
  }
  Thread* resume() {
    Thread* t = threadQueue.front();
    threadQueue.pop_front();
#ifdef MUTEX_NO_BATONPASS
    kernelScheduler.start(*t);
#else
    kernelScheduler.yield(*t);    // yield control to thread t
#endif
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
    ScopedLock<> lo(lk);
    if likely(counter < 1) suspend();
    else counter -= 1;
  }
  bool tryP() {
    ScopedLock<> lo(lk);
    if likely(counter < 1) return false;
    counter -= 1;
    return true;
  }
  void V() {
    lk.acquire();
    if likely(waiting()) {
      resume(); // pass closed lock to thread waiting on V()
    } else {
      counter += 1;
      lk.release();
    }
  }
};

#ifdef MUTEX_NO_BATONPASS
class Mutex : public BlockingSync {
protected:
  Thread* owner;
public:
  Mutex() : owner(nullptr) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(Mutex)); }
  void acquire() {
    ScopedLock<> lo(lk);
    if (owner) suspend();
    else owner = Processor::getCurrThread();
  }
  bool tryAcquire() {
    ScopedLock<> lo(lk);
    if (owner) return false;
    owner = Processor::getCurrThread();
    return true;
  }
  void release() {
    ScopedLock<> lo(lk);
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    if likely(waiting()) {
      owner = resume();
    } else {
      owner = nullptr;
    }
  }
  bool locked() {
    return owner != nullptr;
  }
};
#else
class Mutex : public BlockingSync {
protected:
  Thread* owner;
public:
  Mutex() : owner(nullptr) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(Mutex)); }
  void acquire() {
    ScopedLock<> lo(lk);
    if (owner) suspend();
    owner = Processor::getCurrThread();
  }
  bool tryAcquire() {
    ScopedLock<> lo(lk);
    if (owner) return false;
    owner = Processor::getCurrThread();
    return true;
  }
  void release() {
    lk.acquire();
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    if likely(waiting()) {
      resume();
    } else {
      owner = nullptr;
      lk.release();
    }
  }
  bool locked() {
    return owner != nullptr;
  }
};
#endif

class RecursiveMutex : public BlockingSync {
protected:
  Thread* owner;
  mword recurse;
public:
  RecursiveMutex() : owner(nullptr), recurse(0) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RecursiveMutex)); }
  void acquire() {
    ScopedLock<> lo(lk);
    if (owner && owner != Processor::getCurrThread()) suspend();
    owner = Processor::getCurrThread();
    recurse += 1;
  }
  bool tryAcquire() {
    ScopedLock<> lo(lk);
    if (!owner) owner = Processor::getCurrThread();
    else if (owner != Processor::getCurrThread()) return false;
    recurse += 1;
    return true;
  }
  void release() {
#ifdef MUTEX_NO_BATONPASS
    ScopedLock<> lo(lk);
#else
    lk.acquire();
#endif
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    recurse -= 1;
#ifdef MUTEX_NO_BATONPASS
    if (recurse == 0) {
      if likely(waiting()) {
        owner = resume();
      } else {
        owner = nullptr;
      }
    }
#else
    if (recurse > 0) {
      lk.release();
    } else {
      if likely(waiting()) {
        resume();
      } else {
        owner = nullptr;
        lk.release();
      }
    }
#endif
  }
};

#endif /* _BlockingSync_h_ */
