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

#include "mach/stack.h"
#include "kern/Thread.h"
#include "ipc/SpinLock.h"
#include "extern/stl/mod_list"
#include "extern/stl/mod_set"

class Thread;

extern "C" void isr_handler_0x20();

class Scheduler {
  friend void isr_handler_0x20(); // timerEvent

  SpinLock lock;
  bool enableInterrupts;
  // shared embedded linkage -> thread can only be on one queue at a time
  EmbeddedList<Thread*> readyQueue[2];
  EmbeddedList<Thread*> destroyList;
  EmbeddedMultiset<Thread*,Thread::TimeoutCompare> timerQueue;
  SpinLock timerLock;

  Scheduler(const Scheduler&) = delete;                  // no copy
  const Scheduler& operator=(const Scheduler&) = delete; // no assignment

  void ready(Thread& t) { t.status = Thread::Ready; readyQueue[t.priority].push_back(&t); }
  void schedule(bool ei = false);
  void timerEvent(mword ticks);

public:
  Scheduler() : enableInterrupts(false) {}
  void kill();
  void preempt();
  void sleep(mword t);
  void suspend(SpinLock& rl);
  void suspend();
  void start(Thread& t) {
    ScopedLock<> sl(lock);
    ready(t);
  }
  void run(Thread& t, function_t func, ptr_t data) {
    t.stackPointer = stackInitIndirect(t.stackPointer, func, data, (void*)invoke);
    start(t);
  }
  void runUser(Thread& t, function_t func, ptr_t data) {
    t.stackPointer = stackInitIndirect(t.stackPointer, func, data, (void*)invokeUser);
    start(t);
  }
  static void invoke(function_t func, ptr_t data);
  static void invokeUser(function_t func, ptr_t data);
};

#endif /* _Scheduler_h_ */
