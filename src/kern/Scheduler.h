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
#ifndef _Scheduler_h_
#define _Scheduler_h_ 1

#include "util/EmbeddedQueue.h"
#include "util/SpinLock.h"

class Thread;

class Scheduler {
  friend class Thread; // lk
  volatile SpinLock lk;
  EmbeddedQueue<Thread> readyQueue;
  void ready(Thread& t);
  void schedule();
  void yieldInternal();

public:
  void start(Thread& t) {
    lk.acquire();
    ready(t);
    lk.release();
  }
  void suspend(volatile SpinLock& rl) {
    lk.acquire();
    rl.release();
    schedule();
    lk.release();
  }
  void yield() {
    lk.acquire();
    yieldInternal();
    lk.release();
  }
  void preempt() {
    lk.acquireISR();
    yieldInternal();
    lk.releaseISR();
  }
};

#endif /* _Scheduler_h_ */
