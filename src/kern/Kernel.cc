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
#include "mach/Processor.h"
#include "kern/AddressSpace.h"
#include "kern/Kernel.h"
#include "kern/OutputSafe.h"
#include "kern/Process.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"
#include "world/File.h"

#include "kern/DynamicTimer.h"
#include "kern/TimerEvent.h"

AddressSpace kernelSpace(AddressSpace::Kernel);
KernelHeap kernelHeap;
Scheduler kernelScheduler;
map<kstring, File*, less<kstring>, KernelAllocator<kstring>> kernelFS;

static void mainLoop(ptr_t);  // forward declaration
static void timertest(ptr_t); // forward declaration
static void task(ptr_t);      // forward declaration

static void apIdleLoop() {
  Machine::initAP2();
  for (;;) Halt();
}

static void bspIdleLoop() {
  Machine::initBSP2();
  StdOut.outln("Welcome to KOS!");
  Thread* t = Thread::create(kernelSpace, "0");
  kernelScheduler.run(*t, mainLoop, nullptr);
  for (;;) Pause();
}

extern "C" void kmain(mword magic, mword addr) __section(".boot.text");
extern "C" void kmain(mword magic, mword addr) {
  if (magic == 0 && addr == 0xE85250D6) {
    // start up AP: low-level machine-dependent initialization
    Machine::initAP(apIdleLoop);
  } else {
    // start up BSP: low-level machine-dependent initialization
    Machine::initBSP(magic, addr, bspIdleLoop);
  }
}

struct TimerTest : public TimerEvent {
  TimerTest(mword timeout) : TimerEvent(timeout) {
    DBG::outln(DBG::Basic, "running  dynamic timer test with 5 sec timeout period");
  }
  bool run(mword currentTick) {
    if (timedout()) {
      DBG::outln(DBG::Basic, "timedout!!!");
      return true;
    }
    return false;
  }
};

static void mainLoop(ptr_t) {
  File* f1 = kernelFS.find("motd")->second;
  for (;;) {
    char c;
    if (f1->read(&c, 1) == 0) break;
    StdOut.out(c);
    StdDbg.out(c);
  }

  Process p;
  p.execElfFile("testprogram2");

  DynamicTimer::registerEvent(new TimerTest(5000));

  // TODO: create processes and leave BSP thread waiting for events
  kernelScheduler.run( *Thread::create(kernelSpace, "TT"), timertest, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "A"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "B"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "C"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "D"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "E"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "F"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "G"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "H"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "I"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "J"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "K"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "L"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "M"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "N"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "O"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "P"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "Q"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "R"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "S"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "T"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "U"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "V"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "W"), task, nullptr);
  kernelScheduler.run( *Thread::create(kernelSpace, "X"), task, nullptr);
  task(nullptr);
}

void timertest(ptr_t) {
  StdErr.out(" PIT test, 3 secs...");
  for (int i = 0; i < 3; i++) {
    kernelScheduler.sleep(Machine::now() + 1000);
    StdErr.out(' ', i+1);
  }
  StdErr.outln(" done.");
}

void task(ptr_t) {
  mword id = Processor::getApicID();
  for (;;) {
    StdOut.out(Processor::getCurrThread()->getName(), id, ' ');
    mword newid;
    do {
      Pause();
      newid = Processor::getApicID();
    } while (newid == id);
    id = newid;
  }
  // never reached...
}
