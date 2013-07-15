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
#ifndef _Processor_h_
#define _Processor_h_ 1

#include "util/Output.h"
#include "mach/APIC.h"
#include "mach/CPU.h"
#include "mach/Memory.h"

#include "mach/isr_wrapper.h"

class Thread;
class FrameManager;
class GdbCpu;

// would like to use 'offsetof', but asm does not work with 'offsetof'
// use fs:0 as 'this', then access member: slower, but cleaner?
// see http://stackoverflow.com/questions/3562697/whats-use-of-c-in-x86-64-inline-assembly
class Processor {
  mword         apicID;
  mword         cpuID;
  Thread*       currThread;
  Thread*       idleThread;
  FrameManager* frameManager;
  mword         lockCount;
  GdbCpu*       gdbCpu;

  friend class Machine;
  friend class Gdb;

  // lockCount must not reach 0 during bootstrap -> interrupts disabled
  Processor() : apicID(0), cpuID(0), currThread(nullptr), idleThread(nullptr),
    frameManager(nullptr), lockCount(mwordlimit / 2), gdbCpu(0) {}
  Processor(const Processor&) = delete;            // no copy
  Processor& operator=(const Processor&) = delete; // no assignment

  void init(mword apic, mword cpu, FrameManager& fm) {
    apicID = apic;
    cpuID = cpu;
    frameManager = &fm;
  }

  void install() {
   MSR::write(MSR::GS_BASE, mword(this));
   //Prepare Syscall/Sysret Registers
   initSysCall();
  }

	void initSysCall() {
		MSR::enableSYSCALL();
		MSR::write(MSR::SYSCALL_CSTAR, 0x0);
		MSR::write(MSR::SYSCALL_SFMASK, 0x0);
		MSR::write(MSR::SYSCALL_LSTAR, mword(syscall_handler));
		//uint64_t star = ((((uint64_t)Machine::userCodeSelector|0x3) - 16)<<48)|((uint64_t)Machine::userCodeSelector<<32);
		MSR::write(MSR::SYSCALL_STAR, 0x0008000800000000);
	}

  void initThread(Thread& t) {
   currThread = idleThread = &t;
  }
  static volatile LAPIC* lapic() {
   return (LAPIC*)lapicAddr;
  }
  void setGdbCpu(GdbCpu* s) {
    gdbCpu = s;
  }

public:
  static mword getApicID() {
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, apicID))); return x;
  }
  static mword getCpuID() {
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, cpuID))); return x;
  }
  static Thread* getCurrThread() {
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, currThread))); return (Thread*)x;
  }
  static void setCurrThread(Thread* x) {
             asm volatile("mov %0, %%gs:%c1" :: "r"(x), "i"(offsetof(struct Processor, currThread)) : "memory");
  }
  static Thread* getIdleThread() {
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, idleThread))); return (Thread*)x;
  }
  static FrameManager* getFrameManager() {
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, frameManager))); return (FrameManager*)x;
  }
  static void setFrameManager(FrameManager* x) {
             asm volatile("mov %0, %%gs:%c1" :: "r"(x), "i"(offsetof(struct Processor, frameManager)) : "memory");
  }
  static GdbCpu* getGdbCpu() {
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, gdbCpu))); return (GdbCpu*)x;
  }
  static mword getLockCount() {
    KASSERT0(!interruptsEnabled());
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, lockCount))); return x;
  }
  static mword incLockCount() {
    KASSERT0(!interruptsEnabled());
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, lockCount)) : "memory");
             asm volatile("mov %0, %%gs:%c1" :: "r"(x+1), "i"(offsetof(struct Processor, lockCount)) : "memory");
   return x+1;
  }
  static mword decLockCount() {
    KASSERT0(!interruptsEnabled());
    mword x; asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, lockCount)) : "memory");
             asm volatile("mov %0, %%gs:%c1" :: "r"(x-1), "i"(offsetof(struct Processor, lockCount)) : "memory");
    return x-1;
  }
  void startInterrupts() {
    lockCount = 0;
    CPU::enableInterrupts();
  }
  static void enableInterrupts() {
    if (decLockCount() == 0) CPU::enableInterrupts();
  }
  static void disableInterrupts() {
    CPU::disableInterrupts();
    incLockCount();
  }
  static bool interruptsEnabled() {
    return RFlags::interruptsEnabled();
  }

  static void enableAPIC(uint8_t sv) {
    lapic()->enable(sv);
  }

  static void sendEOI() {
    lapic()->sendEOI();
  }

  void sendInitIPI() {
    mword err = lapic()->sendInitIPI(apicID);
    KASSERT1(err == 0, FmtHex(err));
  }

  void sendStartupIPI(uint8_t vector) {
    mword err = lapic()->sendStartupIPI(apicID, vector);
    KASSERT1(err == 0, FmtHex(err));
  }

  void sendWakeUpIPI() {
    mword err = lapic()->sendIPI(apicID, 0x40 );
    KASSERT1(err == 0, FmtHex(err));
  }

  void sendTestIPI() {
    mword err = lapic()->sendIPI(apicID, 0x41 );
    KASSERT1(err == 0, FmtHex(err));
  }

} __packed;

#endif /* Processor_h_ */
