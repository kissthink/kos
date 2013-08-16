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
#ifndef _Scheduler_h_
#define _Scheduler_h_ 1

#include "extern/stl/mod_list"
#include "extern/stl/mod_set"
#include "ipc/SpinLock.h"
#include "ipc/RecursiveSpinLock.h"

class Thread;

extern "C" void isr_handler_0x20();

struct TimeoutCompare {
  inline bool operator()( const Thread* t1, const Thread* t2 );
};

class Scheduler {
  friend class Thread;            // lk
  friend void isr_handler_0x20(); // timerEvent
  volatile SpinLock lk;
  InPlaceList<Thread*> readyQueue[2];
  InPlaceMultiset<Thread*,0,TimeoutCompare> timerQueue;
  void ready(Thread& t);
  void schedule();
  void yieldInternal();
  void timerEvent(mword ticks);

  Scheduler(const Scheduler&) = delete;                  // no copy
  const Scheduler& operator=(const Scheduler&) = delete; // no assignment

public:
  Scheduler() = default;
  void start(Thread& t) {
    ScopedLock<> lo(lk);
    ready(t);
  }
  void suspend(volatile SpinLock& rl) {
    ScopedLock<> lo(lk);
    rl.release();
    schedule();
  }
  void suspend(volatile RecursiveSpinLock& rl) {
    ScopedLock<> lo(lk);
    rl.release();
    KASSERT1(!rl.isOwner(), "still being recursed");
    schedule();
  }
  void suspend() {
    ScopedLock<> lo(lk);
    schedule();
  }
  int sleep(Thread& t);
  int sleep(Thread& t, volatile SpinLock& rl);
  void yield();
  void yield(Thread& t);
};

#endif /* _Scheduler_h_ */
