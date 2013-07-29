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
    kernelScheduler.start(*t);
    return t;
  }
  bool waiting() {
    return !threadQueue.empty();
  }
};

class RwBlockingSync {
  InPlaceList<Thread*> readerQueue;
  InPlaceList<Thread*> writerQueue;
protected:
  SpinLock lk;
  ~RwBlockingSync() {
    ScopedLock<> lo(lk);
    while (!readerQueue.empty()) resumeReader();
    while (!writerQueue.empty()) resumeWriter();
  }
  void suspend(bool writer=false) {
    if (writer) writerQueue.push_back(Processor::getCurrThread());
    else readerQueue.push_back(Processor::getCurrThread());
    kernelScheduler.suspend(lk);
  }
  Thread* resumeReader() {
    Thread* t = readerQueue.front();
    readerQueue.pop_front();
    kernelScheduler.start(*t);
    return t;
  }
  Thread* resumeWriter() {
    Thread* t = writerQueue.front();
    writerQueue.pop_front();
    kernelScheduler.start(*t);
    return t;
  }
  bool waiting(bool writer=false) {
    if (writer) return !writerQueue.empty();
    else return !readerQueue.empty();
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

class Mutex : public BlockingSync {
protected:
  Thread* owner;
public:
  Mutex() : owner(nullptr) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(Mutex)); }
  void acquire() {
    ScopedLock<> lo(lk);
    if (owner) {
      KASSERT1(owner != Processor::getCurrThread(), "attempt to recursively acquire a non-recursive mutex");
      suspend();
    } else owner = Processor::getCurrThread();
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
      owner = resume();
    } else {
      owner = nullptr;
      lk.release();
    }
  }
};

class RecursiveMutex : public BlockingSync {
protected:
  Thread* owner;
  int recurse;
public:
  RecursiveMutex() : owner(nullptr), recurse(0) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RecursiveMutex)); }
  void acquire() {
    ScopedLock<> lo(lk);
    if (owner) {
      if (owner != Processor::getCurrThread()) suspend();
      recurse += 1;
    } else owner = Processor::getCurrThread();
  }
  bool tryAcquire() {
    ScopedLock<> lo(lk);
    if (owner) {
      if (owner != Processor::getCurrThread()) return false;
      recurse += 1;
    }
    owner = Processor::getCurrThread();
    return true;
  }
  void release() {
    lk.acquire();
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    if (!recurse) {
      if likely(waiting()) owner = resume();
      else {
        owner = nullptr;
        lk.release();
      }
    } else {
      recurse -= 1;
      lk.release();
    }
  }
};

class RwMutex : public RwBlockingSync {
protected:
  Thread* owner;
  uint64_t shared;
  bool exclusive;
  SpinLock lk;
public:
  RwMutex() : owner(nullptr), shared(0), exclusive(false) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(RwMutex)); }
  void readAcquire() {
    ScopedLock<> lo(lk);
    if (exclusive) suspend();
    shared += 1;
  }
  void writeAcquire() {
    ScopedLock<> lo(lk);
    if (exclusive) {
      KASSERT1(owner != Processor::getCurrThread(),
        "attempt to recursively acquire non-recursive mutex");
      suspend(true);
    } else if (shared) suspend(true);
    owner = Processor::getCurrThread();
    exclusive = true;
  }
  bool tryReadAcquire() {
    ScopedLock<> lo(lk);
    if (exclusive) return false;
    shared += 1;
    return true;
  }
  bool tryWriteAcquire() {
    ScopedLock<> lo(lk);
    if (exclusive || shared) return false;
    owner = Processor::getCurrThread();
    exclusive = true;
    return true;
  }
  void readRelease() {
    lk.acquire();
    KASSERT1(!exclusive, "release a read lock while write lock is held");
    shared -= 1;
    if likely(waiting()) resumeReader();
    else lk.release();
  }
  void writeRelease() {
    lk.acquire();
    KASSERT1(owner == Processor::getCurrThread(), "attempt to release lock by non-owner");
    KASSERT1(!shared, "release a write lock while read lock is held");
    if likely(waiting(true)) {
      owner = resumeWriter();
    } else {
      exclusive = false;
      owner = nullptr;
      if (waiting()) resumeReader();
      else lk.release();
    }
  }
  bool tryUpgrade() {
    ScopedLock<> lo(lk);
    if (shared != 1) return false;
    owner = Processor::getCurrThread();
    exclusive = true;
    shared = 0;
    return true;
  }
  void downGrade() {
    lk.acquire();
    KASSERT1(exclusive, "cannot downgrade read lock");
    if (!waiting()) {
      exclusive = false;
      owner = nullptr;
      shared = 1;
      lk.release();
    } else {    // poor form of broadcast
      while (waiting()) resumeReader();
    }
  }
};


#endif /* _BlockingSync_h_ */
