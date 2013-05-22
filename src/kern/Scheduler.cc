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
#include "util/Log.h"
#include "mach/Processor.h"
#include "mach/stack.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"

void Scheduler::ready(Thread& t) {
  readyQueue[t.getPrio()].push(&t);
}

// extremely simple 2-class prio scheduling!
void Scheduler::schedule() {
  Thread* nextThread;
  Thread* prevThread = Processor::getCurrThread();
  if unlikely(!readyQueue[1].empty()) {
    nextThread = readyQueue[1].front();
    readyQueue[1].pop();
  } else if likely(!readyQueue[0].empty()) {
    nextThread = readyQueue[0].front();
    readyQueue[0].pop();
  } else {
    nextThread = Processor::getIdleThread();
  }
  Processor::setCurrThread(nextThread);
  KASSERT(Processor::getLockCount() == 1, Processor::getLockCount());
  DBG::outln(DBG::Scheduler, "CPU ", Processor::getApicID(), " switch: ");
  if (prevThread->getName()) DBG::outln(DBG::Scheduler, prevThread->getName());
  else DBG::outln(DBG::Scheduler, prevThread);
  DBG::outln(DBG::Scheduler, " -> ");
  if (nextThread->getName()) DBG::outln(DBG::Scheduler, nextThread->getName());
  else DBG::outln(DBG::Scheduler, nextThread);

  // this needs to happen in kernel memory?
  stackSwitch(&prevThread->stackPointer, &nextThread->stackPointer);
  // AS switch, if necessary...
}

void Scheduler::yieldInternal() {
  if likely(Processor::getCurrThread() != Processor::getIdleThread()) {
    ready(*Processor::getCurrThread());
  }
  schedule();
}
