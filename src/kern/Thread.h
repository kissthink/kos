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
#ifndef _Thread_h_
#define _Thread_h_ 1

#include "util/EmbeddedQueue.h"
#include "mach/platform.h"
#include "mach/stack.h"
#include "kern/Kernel.h"
#include "kern/Scheduler.h"

class AddressSpace;

class Thread : public EmbeddedElement<Thread> {
  static const size_t defaultStack = 2 * pagesize<1>();
  friend class Scheduler; // stackPointer
  AddressSpace* addressSpace;
  vaddr stackPointer;
  size_t stackSize;
  int prio;
  const char* name;

  Thread(AddressSpace& as, vaddr sp, size_t s) : addressSpace(&as), stackPointer(sp), stackSize(s), prio(0), name(nullptr) {}
  ~Thread() { /* join */ }
  static void invoke( function_t func, ptr_t data );

public:
  Thread* setPrio(int p) { prio = p; return this; }
  int getPrio() const { return prio; }
  Thread* setName(const char *n) { name = n; return this; }
  const char* getName() const { return name; }
  AddressSpace& getAddressSpace() const { return *addressSpace; }

  static Thread* create(AddressSpace& as, size_t stackSize = defaultStack) {
    vaddr mem = kernelVM.alloc(stackSize) + stackSize - sizeof(Thread);
    return new (ptr_t(mem)) Thread(as, mem, stackSize);
  }
  static Thread* create(function_t func, ptr_t data, AddressSpace& as, size_t stackSize = defaultStack) {
    Thread* t = create(as, stackSize);
    t->run(func, data);
    return t;
  }
  static void destroy(Thread* t) {
    t->~Thread();
    vaddr mem = vaddr(t) + sizeof(Thread) - t->stackSize;
    kernelVM.release(mem, t->stackSize);
  }
  void run(function_t func, ptr_t data) {
    stackPointer = stackInitIndirect(stackPointer, func, data, (void*)Thread::invoke);
    kernelScheduler.start(*this);
  }
  void runDirect(funcvoid_t func) { // combine stackInitSimple and stackStart?
    stackPointer = stackInitSimple(stackPointer, func);
    stackStart(stackPointer);
  }
} __packed;

#endif /* _Thread_h_ */
