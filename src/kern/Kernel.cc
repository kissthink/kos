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
#include "util/OutputSafe.h"
#include "mach/Machine.h"
#include "mach/Processor.h"
#include "kern/AddressSpace.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"
#include "world/File.h"
#include "world/ELFLoader.h"

AddressSpace kernelSpace(AddressSpace::Kernel);
KernelVM kernelVM;
Scheduler kernelScheduler;
map<kstring,File*,std::less<kstring>,KernelAllocator<kstring>> kernelFS;

static void mainLoop(ptr_t); // forward declaration
static void task(ptr_t);     // forward declaration

void apIdleLoop() {
  Machine::initAP2();
  for (;;) Halt();
}

void bspIdleLoop() {
  Machine::initBSP2();
  StdOut.outln("Welcome to KOS!");
  Thread::create(mainLoop, nullptr, kernelSpace, "BSP");
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
};

void mainLoop(ptr_t) {
  File* f1 = kernelFS.find("motd")->second;
  for (;;) {
    char c;
    if (f1->read( &c, 1 ) == 0) break;
    StdOut.out(c);
    StdDbg.out(c);
  }

	File* f = kernelFS.find("testprogram2")->second;

	//User Address Space
	AddressSpace userAS(AddressSpace::User);
	vaddr startAS = pagesize<2>();
	vaddr lengthAS = topuser - startAS;
	userAS.setMemoryRange(startAS,lengthAS);

	//ELF Loader
	ELFLoader elfLoader;
	if (elfLoader.loadAndMapELF(f,&userAS)) {
		//Clone and Activate Address Space
		userAS.clonePagetable(kernelSpace);
		userAS.activate();

		//Execute Program
		int (*func_ptr)(void);
		func_ptr = (int (*)(void))elfLoader.findMainAddress();
		KASSERT0(vaddr(func_ptr) != topaddr);
		StdOut.outln("return value: ", func_ptr());

		//Activate Kernel Space again
		kernelSpace.activate();
	}

  Breakpoint();
  // TODO: create processes and leave BSP thread waiting for events
  Thread::create(task, nullptr, kernelSpace, "A");
  Thread::create(task, nullptr, kernelSpace, "B");
  Thread::create(task, nullptr, kernelSpace, "C");
  Thread::create(task, nullptr, kernelSpace, "D");
  Thread::create(task, nullptr, kernelSpace, "E");
  Thread::create(task, nullptr, kernelSpace, "F");
  Thread::create(task, nullptr, kernelSpace, "G");
  Thread::create(task, nullptr, kernelSpace, "H");
  Thread::create(task, nullptr, kernelSpace, "I");
  Thread::create(task, nullptr, kernelSpace, "J");
  Thread::create(task, nullptr, kernelSpace, "K");
  Thread::create(task, nullptr, kernelSpace, "L");
  Thread::create(task, nullptr, kernelSpace, "M");
  Thread::create(task, nullptr, kernelSpace, "N");
  Thread::create(task, nullptr, kernelSpace, "O");
  Thread::create(task, nullptr, kernelSpace, "P");
  Thread::create(task, nullptr, kernelSpace, "Q");
  Thread::create(task, nullptr, kernelSpace, "R");
  Thread::create(task, nullptr, kernelSpace, "S");
  Thread::create(task, nullptr, kernelSpace, "T");
  Thread::create(task, nullptr, kernelSpace, "U");
  Thread::create(task, nullptr, kernelSpace, "V");
  Thread::create(task, nullptr, kernelSpace, "W");
  Thread::create(task, nullptr, kernelSpace, "X");
  task(nullptr);
}

static SpinLock lk;

void task(ptr_t) {
  mword id = Processor::getApicID();
  for (;;) {
    lk.acquire();
    StdOut.out(Processor::getCurrThread()->getName(), id, ' ');
//    StdDbg.out(Processor::getCurrThread()->getName(), id, ' ');
    lk.release();
    mword newid;
    do {
      Pause();
      newid = Processor::getApicID();
    } while (newid == id);
    id = newid;
  }
  // never reached...
}
