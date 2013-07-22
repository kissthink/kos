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
#include "kern/Scheduler.h"
#include "kern/Thread.h"

inline bool TimeoutCompare::operator()( const Thread* t1, const Thread* t2 ) {
  return t1->timeout < t2->timeout;
}

void Scheduler::ready(Thread& t) {
  readyQueue[t.getPrio()].push_back(&t);
}

// extremely simple 2-class prio scheduling!
void Scheduler::schedule() {
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
  Processor::setCurrThread(nextThread);
  KASSERT1(Processor::getLockCount() == 1, Processor::getLockCount());
  DBG::out(DBG::Scheduler, "CPU ", Processor::getApicID(), " switch: ");
  if (prevThread->getName()) DBG::out(DBG::Scheduler, prevThread->getName());
  else DBG::out(DBG::Scheduler, prevThread);
  DBG::out(DBG::Scheduler, " -> ");
  if (nextThread->getName()) DBG::outln(DBG::Scheduler, nextThread->getName());
  else DBG::outln(DBG::Scheduler, nextThread);

  // this needs to happen in kernel memory?
  stackSwitch(&prevThread->stackPointer, &nextThread->stackPointer);
  // AS switch, if necessary...
}

void Scheduler::timerEvent(mword ticks) {
  ScopedLock<> lo(lk);
  InPlaceSet<Thread*,0,TimeoutCompare>::iterator i = timerQueue.begin();
  if (i != timerQueue.end() && ticks >= (*i)->timeout) {
    Thread* t = *i;
    timerQueue.erase(i);
    ready(*t);
  }
}

void Scheduler::sleep(Thread& t) {
  ScopedLock<> lo(lk); 
  timerQueue.insert(&t);
  schedule();
}

void Scheduler::yield() {
  ScopedLock<> lo(lk);
  if likely(Processor::getCurrThread() != Processor::getIdleThread()) {
    ready(*Processor::getCurrThread());
  }
  schedule();
}
