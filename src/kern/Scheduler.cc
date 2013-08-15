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
#include "mach/Processor.h"
#include "mach/stack.h"
#include "kern/Debug.h"
#include "kern/Kernel.h"
#include "kern/Scheduler.h"

// extremely simple 2-class prio scheduling!
void Scheduler::schedule(bool ei) {
  Thread* nextThread;
  Thread* prevThread = Processor::getCurrThread();
  if unlikely(!readyQueue[1].empty()) {
    nextThread = readyQueue[1].front();
    readyQueue[1].pop_front();
  } else if likely(!readyQueue[0].empty()) {
    nextThread = readyQueue[0].front();
    readyQueue[0].pop_front();
  } else {
    nextThread = Processor::getIdleThread();
  }

  if (prevThread == nextThread) return;
  Processor::setCurrThread(nextThread);

  KASSERT1(Processor::getLockCount() == 1, Processor::getLockCount());
  DBG::out(DBG::Scheduler, "CPU ", Processor::getApicID(), " switch: ");
  if (prevThread->getName()) DBG::out(DBG::Scheduler, prevThread->getName());
  else DBG::out(DBG::Scheduler, prevThread);
  DBG::out(DBG::Scheduler, " -> ");
  if (nextThread->getName()) DBG::outln(DBG::Scheduler, nextThread->getName());
  else DBG::outln(DBG::Scheduler, nextThread);

  enableInterrupts = ei;

  if ( nextThread->getAddressSpace() != &kernelSpace
    && nextThread->getAddressSpace() != prevThread->getAddressSpace() ) {
    stackSwitchAS(&prevThread->stackPointer, &nextThread->stackPointer, nextThread->getAddressSpace()->getPagetable());
  } else {
    stackSwitch(&prevThread->stackPointer, &nextThread->stackPointer);
  }

  while (!destroyList.empty()) {
    Thread* t = destroyList.front();
    destroyList.pop_front();
    t->destroy();
  }
}

void Scheduler::timerEvent(mword ticks) {
  ScopedLock<> sl(timerLock);
  for (;;) {
    EmbeddedMultiset<Thread*,Thread::TimeoutCompare>::iterator i = timerQueue.begin();
  if (i == timerQueue.end() || ticks < (*i)->timeout) break;
    Thread* t = *i;
    timerQueue.erase(i);
    timerLock.release();
    start(*t);
    timerLock.acquire();
  }
}

// TODO: clean up AS/Process when last thread is gone...
void Scheduler::kill() {
  ScopedLock<> sl(lock);
  destroyList.push_back(Processor::getCurrThread());
  schedule();                                    // does not return
}

void Scheduler::preempt() {
  ScopedLock<> sl(lock);
  if likely(Processor::getCurrThread() != Processor::getIdleThread()) {
    ready(*Processor::getCurrThread());
  }
  schedule(true);
}

void Scheduler::sleep(mword t) {
  timerLock.acquire();
  Processor::getCurrThread()->timeout = t;
  timerQueue.insert(Processor::getCurrThread());
  suspend(timerLock);
}

void Scheduler::suspend() {
  lock.acquire();
  schedule();
  lock.release(enableInterrupts);
}

void Scheduler::suspend(SpinLock& rl) {
  lock.acquire();
  rl.release();
  schedule();
  lock.release(enableInterrupts);
}

void Scheduler::invoke(function_t func, ptr_t data) {
  KASSERT1(Processor::getLockCount() == 1, Processor::getLockCount());
  kernelScheduler.lock.release(kernelScheduler.enableInterrupts);
  func(data);
  kernelScheduler.kill();                        // does not return
}

void Scheduler::invokeUser(function_t func, ptr_t data) {
  KASSERT1(Processor::getLockCount() == 1, Processor::getLockCount());
  kernelScheduler.lock.release(kernelScheduler.enableInterrupts);
  asm volatile("movq %0, %%rcx"::"g"(func) : "rcx", "memory");
  asm volatile("sysretq" ::: "memory");          // does not return
}
