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
#include "mach/stack.h"
#include "kern/AddressSpace.h"
#include "kern/Thread.h"

void Thread::destroy() {
  // wake up joiners (keep in list)?
  vaddr mem = vaddr(this) + sizeof(Thread) - stackSize;
  addressSpace->releasePages<1>(mem, stackSize);
}

Thread* Thread::create(AddressSpace& as, size_t stackSize) {
  vaddr mem = as.allocPages<1>(stackSize, AddressSpace::Data) + stackSize - sizeof(Thread);
  return new (ptr_t(mem)) Thread(as, mem, stackSize);
}

Thread* Thread::create(AddressSpace& as, const char *n, size_t stackSize) {
  vaddr mem = as.allocPages<1>(stackSize, AddressSpace::Data) + stackSize - sizeof(Thread);
  return new (ptr_t(mem)) Thread(as, mem, stackSize, n);
}

void Thread::runDirect(funcvoid_t func) {
  stackPointer = stackInitSimple(stackPointer, func);
  status = Running;
  stackStart(stackPointer);
}

#if 0
void Thread::sendEvent(Event* event) {
  ScopedLock<> lo(lk);
  eventQueue.pushBack(event);
  if (status == Sleeping) {
    status = Ready;
    kernelScheduler.start(*this);
  }
}
#endif
