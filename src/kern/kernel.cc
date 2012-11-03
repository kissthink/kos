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
#include "mach/Machine.h"
#include "mach/Processor.h"
#include "kern/AddressSpace.h"
#include "kern/KernelHeap.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"

// declared in Kernel.h
AddressSpace kernelSpace;
Scheduler kernelScheduler;

// declared in globals.h
void globaldelete(ptr_t ptr, size_t size) { KernelHeap::free(ptr,size); }

ptr_t operator new(size_t size) { return KernelHeap::malloc(size); }
ptr_t operator new[](size_t size) { return KernelHeap::malloc(size); }
void operator delete(ptr_t ptr) { KASSERT(false, "delete" ); }
void operator delete[](ptr_t ptr) { KASSERT(false, "delete[]" ); }

static char A = 'A';
static char B = 'B';
static char C = 'C';
static char D = 'D';
static char E = 'E';
static char F = 'F';
static char M = 'M';

static void idleLoop();
static void mainLoop(ptr_t);
static void task(ptr_t);
static void apIdleLoop();

void startGdb();

extern "C" void kmain(mword magic, mword addr) __section(".kernel.init");
extern "C" void kmain(mword magic, mword addr) {
  if (magic == 0 && addr == 0xE85250D6 + KERNBASE) {
    // start up AP and confirm to BSP
    Machine::initAP(apIdleLoop);
  } else {
    // start up BSP: low-level machine-dependent initializations
  	Machine::init(magic, addr, idleLoop);
  }
};

void idleLoop() {
  startGdb();
  Machine::init2();
  kcout << "Welcome to KOS!\n";
  kcout << "Running GDB!\n";
  Thread::create(mainLoop, &M);
  for (;;) Pause();
}

void mainLoop(ptr_t c) {
  Breakpoint();
  Thread::create(task, &A);
	Thread::create(task, &B);
	Thread::create(task, &C);
	Thread::create(task, &D);
	Thread::create(task, &E);
	Thread::create(task, &F);
	task(c);
}

static SpinLock lk;

void task(ptr_t c) {
  for (;;) {
    lk.acquire();
    kcout << *(char*)c << Processor::getApicID() << ' ';
    lk.release();
    for (int i = 0; i < 2500; i += 1) Pause();
  }
  // never reached...
}

void apIdleLoop() {
  Machine::initAP2();
  for (;;) Halt();
}
