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
#include "mach/Machine.h"
#include "kern/Kernel.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"

void Thread::invoke(function_t func, ptr_t data) {
  KASSERT1(Processor::getLockCount() == 1, Processor::getLockCount());
  kernelScheduler.lk.release();
  func(data);
  /* sync/lock with join */
  kernelScheduler.suspend();
}

Thread* Thread::create(AddressSpace& as, size_t stackSize) {
  vaddr mem = kernelHeap.alloc(stackSize) + stackSize - sizeof(Thread);
  return new (ptr_t(mem)) Thread(as, mem, stackSize);
}

Thread* Thread::create(AddressSpace& as, const char *n, size_t stackSize) {
  vaddr mem = kernelHeap.alloc(stackSize) + stackSize - sizeof(Thread);
  return new (ptr_t(mem)) Thread(as, mem, stackSize, n);
}

Thread* Thread::create(function_t func, ptr_t data, AddressSpace& as, size_t stackSize) {
  Thread* t = create(as, stackSize);
  t->run(func, data);
  return t;
}

Thread* Thread::create(function_t func, ptr_t data, AddressSpace& as, const char *n, size_t stackSize) {
  Thread* t = create(as, n, stackSize);
  t->run(func, data);
  return t;
}

void Thread::destroy(Thread* t) {
  t->~Thread();
  vaddr mem = vaddr(t) + sizeof(Thread) - t->stackSize;
  kernelHeap.release(mem, t->stackSize);
}

void Thread::run(function_t func, ptr_t data) {
  stackPointer = stackInitIndirect(stackPointer, func, data, (void*)Thread::invoke);
  kernelScheduler.start(*this);
}

void Thread::runDirect(funcvoid_t func) {
  stackPointer = stackInitSimple(stackPointer, func);
  stackStart(stackPointer);
}

int Thread::sleep(mword t) {
  timeout = Machine::now() + t;
  return kernelScheduler.sleep(*this);
}

int Thread::sleep(mword t, volatile SpinLock& rl) {
  timeout = Machine::now() + t;
  return kernelScheduler.sleep(*this, rl);
}
