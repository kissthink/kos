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

class Processor {
  mword          apicID;
  mword          cpuID;
  Thread*        currThread;
  Thread*        idleThread;
  FrameManager*  frameManager;
  volatile mword doNotPreempt;
  volatile mword lockCount;
  GdbCpu*        gdbCpu;

  static constexpr volatile LAPIC* apic() { return (LAPIC*)lapicAddr; }

  friend class Machine;

  // start with interrupts disable -> lockCount = 1
  Processor() : apicID(0), cpuID(0), currThread(nullptr), idleThread(nullptr),
    frameManager(nullptr), doNotPreempt(0), lockCount(0), gdbCpu(nullptr) {}
  Processor(const Processor&) = delete;            // no copy
  Processor& operator=(const Processor&) = delete; // no assignment

  void init(mword apic, mword cpu) {
    apicID = apic;
    cpuID = cpu;
  }

  void install(FrameManager& fm) {
    frameManager = &fm;
    MSR::enableNX();                               // enable NX paging bit
    CPU::writeCR4(CPU::readCR4() | CPU::PGE());    // enable  G paging bit

    // write GS
    MSR::write(MSR::GS_BASE, mword(this));

    // prepare syscall/sysret registers
		MSR::enableSYSCALL();
		MSR::write(MSR::SYSCALL_CSTAR, 0x0);
		MSR::write(MSR::SYSCALL_SFMASK, 0x0);
		MSR::write(MSR::SYSCALL_LSTAR, mword(syscall_handler));
		//uint64_t star = ((((uint64_t)Machine::userCodeSelector|0x3) - 16)<<48)|((uint64_t)Machine::userCodeSelector<<32);
		MSR::write(MSR::SYSCALL_STAR, 0x0008000800000000);
	}

  void init2(Thread& t, GdbCpu* s) {
    currThread = idleThread = &t;
    gdbCpu = s;
  }

public:
  mword rApicID() {
    return apicID;
  }
  static mword getApicID() {
    mword x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, apicID)));
    return x;
  }
  static mword getCpuID() {
    mword x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, cpuID)));
    return x;
  }
  static Thread* getCurrThread() {
    Thread* x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, currThread)));
    return x;
  }
  static void setCurrThread(Thread* x) {
    asm volatile("mov %0, %%gs:%c1" :: "r"(x), "i"(offsetof(struct Processor, currThread)) : "memory");
  }
  static Thread* getIdleThread() {
    Thread* x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, idleThread)));
    return x;
  }
  static FrameManager* getFrameManager() {
    FrameManager* x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, frameManager)));
    return x;
  }
  static GdbCpu* getGdbCpu() {
    GdbCpu* x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, gdbCpu)));
    return x;
  }
  static bool preempt() {
    mword x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, doNotPreempt)));
    return !x;
  }
  static mword getLockCount() {
    mword x;
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, lockCount)));
    return x;
  }
  static mword incLockCount() {
    asm volatile("mov %0, %%gs:%c1" :: "r"(1), "i"(offsetof(struct Processor, doNotPreempt)) : "memory");
    mword x;
    // TODO: can use add instruction?
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, lockCount)) : "memory");
    asm volatile("mov %0, %%gs:%c1" :: "r"(x+1), "i"(offsetof(struct Processor, lockCount)) : "memory");
    return x+1;
  }
  static mword decLockCount() {
    mword x;
    // TODO: can use sub instruction?
    asm volatile("mov %%gs:%c1, %0" : "=r"(x) : "i"(offsetof(struct Processor, lockCount)) : "memory");
    asm volatile("mov %0, %%gs:%c1" :: "r"(x-1), "i"(offsetof(struct Processor, lockCount)) : "memory");

    asm volatile("mov %0, %%gs:%c1" :: "r"(x-1), "i"(offsetof(struct Processor, doNotPreempt)) : "memory");
    return x-1;
  }
  static void enableInterrupts(){
   KASSERT0(!interruptsEnabled());
   CPU::enableInterrupts();
  }
  static bool interruptsEnabled() {
   return RFlags::interruptsEnabled();
  }

  static void enableAPIC(uint8_t sv) {
    apic()->enable(sv);
  }

  static uint8_t getLAPIC_ID() {
    return apic()->getLAPIC_ID();
  }

  static void sendEOI() {
    apic()->sendEOI();
  }

  void sendInitIPI() {
    mword err = apic()->sendInitIPI(apicID);
    KASSERT1(err == 0, FmtHex(err));
  }

  void sendStartupIPI(uint8_t vector) {
    mword err = apic()->sendStartupIPI(apicID, vector);
    KASSERT1(err == 0, FmtHex(err));
  }

  void sendWakeUpIPI() {
    mword err = apic()->sendIPI(apicID, 0x40 );
    KASSERT1(err == 0, FmtHex(err));
  }

  void sendTestIPI() {
    mword err = apic()->sendIPI(apicID, 0x41 );
    KASSERT1(err == 0, FmtHex(err));
  }

  static uint32_t sendIPI( uint8_t dest, uint8_t vec ) {
    return apic()->sendIPI(dest, vec);
  }

  static void checkCapabilities(bool print)            __section(".boot.text");
} __packed;

#endif /* Processor_h_ */
