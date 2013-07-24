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

#include "extern/stl/mod_set"
#include "mach/platform.h"
#include "mach/stack.h"

class AddressSpace;

class Thread : public mod_set_elem<Thread*> {
public:
  static const size_t defaultStack = 2 * pagesize<1>();
private:
  friend struct TimeoutCompare; // timeout 
  friend class Scheduler; // stackPointer
  AddressSpace* addressSpace;
  vaddr stackPointer;
  size_t stackSize;
  mword timeout;
  int prio;
  const char* name;

  Thread(AddressSpace& as, vaddr sp, size_t s, const char* n = nullptr)
    : addressSpace(&as), stackPointer(sp), stackSize(s), timeout(0), prio(0), name(n) {}
  ~Thread() { /* join */ }
  static void invoke( function_t func, ptr_t data );

public:
  Thread* setPrio(int p) { prio = p; return this; }
  int getPrio() const { return prio; }
  const char* getName() const { return name; }
  AddressSpace& getAddressSpace() const { return *addressSpace; }

  static Thread* create(AddressSpace& as, size_t stackSize = defaultStack);
  static Thread* create(AddressSpace& as, const char *n, size_t stackSize = defaultStack);
  static Thread* create(function_t func, ptr_t data, AddressSpace& as, size_t stackSize = defaultStack);
  static Thread* create(function_t func, ptr_t data, AddressSpace& as, const char *n, size_t stackSize = defaultStack);
  static void destroy(Thread* t);
  void run(function_t func, ptr_t data);
  void runDirect(funcvoid_t func);
  void sleep(mword t);
} __packed;

#endif /* _Thread_h_ */
