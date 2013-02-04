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
#ifndef _Thread_h_
#define _Thread_h_ 1

#include "util/EmbeddedQueue.h"
#include "mach/platform.h"
#include "mach/stack.h"
#include "kern/Kernel.h"
#include "kern/KernelHeap.h"
#include "kern/Scheduler.h"

class Thread : public EmbeddedElement<Thread> {
  static const size_t defaultStack = 2 * pagesize<1>();
  friend class Scheduler; // stackPointer
  vaddr stackPointer;
  size_t stackSize;

  Thread(vaddr sp, size_t s) : stackPointer(sp), stackSize(s) {}
  ~Thread() {}
  static void invoke( function_t func, ptr_t data );

public:
  static Thread* create(size_t stackSize = defaultStack ) {
    vaddr mem = KernelHeap::alloc(stackSize) + stackSize - sizeof(Thread);
    return new (ptr_t(mem)) Thread(mem, stackSize);
  }
  static Thread* create(function_t func, ptr_t data, size_t stackSize = defaultStack ) {
    Thread* t = create(stackSize);
    t->run(func, data);
    return t;
  }
  static void destroy(Thread* t) {
    t->~Thread();
    vaddr mem = vaddr(t) + sizeof(Thread) - t->stackSize;
    KernelHeap::release(mem, t->stackSize);
  }
  void run(function_t func, ptr_t data) {
    stackPointer = stackInitIndirect(stackPointer, func, data, (void*)Thread::invoke);
    kernelScheduler.start(*this);
  }
  void runDirect(funcvoid_t func) {
    stackPointer = stackInitSimple(stackPointer, func);
    stackStart(stackPointer);
  }
} __packed;

#endif /* _Thread_h_ */
