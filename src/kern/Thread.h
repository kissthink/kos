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
#include "util/basics.h"
#include "kern/Event.h"
#include "util/Output.h"

class AddressSpace;
class Scheduler;

class Thread : public mod_set_elem<Thread*> {
public:
  static const size_t defaultStack = 2 * pagesize<1>();
  struct TimeoutCompare {
    inline bool operator()( const Thread* t1, const Thread* t2 ) {
      return t1->timeout < t2->timeout;
    }
  };
private:
  friend class Scheduler; // stackPointer, timeout, priority
  const char* name;
  AddressSpace* addressSpace;
  size_t stackSize;
  vaddr stackPointer;
  mword timeout;
  int priority;
  int errorNo;
  enum Status {
    Ready,
    Running,
    Sleeping,
    Zombie
  };
  Status status;

  Thread(AddressSpace& as, vaddr sp, size_t s, const char* n = nullptr)
    : name(n), addressSpace(&as), stackSize(s), stackPointer(sp),
      timeout(0), priority(0) {}
  ~Thread() = delete;

  void destroy();

public:
  Thread* setPriority(int p) { priority = p; return this; }
  const char* getName() const { return name; }
  AddressSpace* getAddressSpace() const { return addressSpace; }

  static Thread* create(AddressSpace& as, size_t stackSize = defaultStack);
  static Thread* create(AddressSpace& as, const char *n, size_t stackSize = defaultStack);
  void runDirect(funcvoid_t func);

  void sendEvent(Event* event) {
    ABORT1("unimplemented");
  }
  int getErrno() const {
    return errorNo;
  }
  void setErrno(int err) {
    errorNo = err;
  }
} __packed;

#endif /* _Thread_h_ */
