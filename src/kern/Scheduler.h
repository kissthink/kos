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

#include "util/EmbeddedQueue.h"
#include "ipc/SpinLock.h"

class Thread;

class Scheduler {
  friend class Thread; // lk
  volatile SpinLock lk;
  EmbeddedQueue<Thread> readyQueue[2];
  void ready(Thread& t);
  void schedule();
  void yieldInternal();

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
  void suspend() {
    ScopedLock<> lo(lk);
    schedule();
  }
  void yield() {
    ScopedLock<> lo(lk);
    yieldInternal();
  }
};

#endif /* _Scheduler_h_ */
