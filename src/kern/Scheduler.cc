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
#include "util/Log.h"
#include "mach/Processor.h"
#include "mach/stack.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"

void Scheduler::ready(Thread& t) {
  readyQueue.push(&t);
}

void Scheduler::schedule() {
  Thread* nextThread;
  Thread* prevThread = Processor::getThread();
  if likely(!readyQueue.empty()) {
    nextThread = readyQueue.front();
    readyQueue.pop();
  } else {
    nextThread = Processor::getIdleThread();
  }
  Processor::setThread(nextThread);
  KASSERT(Processor::getLockCount() == 1, Processor::getLockCount());
  DBG::outln(DBG::Scheduler, "switch on ", Processor::getApicID(), ':', prevThread, " -> ", nextThread );
  stackSwitch(&prevThread->stackPointer, &nextThread->stackPointer);
}

void Scheduler::yieldInternal() {
  if likely(Processor::getThread() != Processor::getIdleThread()) {
    ready(*Processor::getThread());
  }
  schedule();
}
