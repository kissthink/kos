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
#ifndef _Processor_h_
#define _Processor_h_ 1

#include "util/Log.h"
#include "mach/APIC.h"
#include "mach/CPU.h"
#include "mach/Memory.h"

class Thread;
class FrameManager;

// would like to use 'offsetof', but asm does not work with 'offsetof'
// use fs:0 as 'this', then access member: slower, but cleaner?

class Processor {
  mword         apicID;
  mword         cpuID;
  Thread*       thread;
  Thread*       idleThread;
  FrameManager* frameManager;
  mword					lockCount;

  static volatile LAPIC* lapic() {
    return (LAPIC*)lapicAddr;
  }

public:
  // lockCount must not reach 0 -> interrupts disabled during bootstrap
  Processor() : apicID(0), cpuID(0), thread(nullptr), idleThread(nullptr),
    frameManager(nullptr), lockCount(mwordlimit / 2) {}
  Processor(const Processor&) = delete;            // no copy
  Processor& operator=(const Processor&) = delete; // no assignment

  void init(mword apic, mword cpu, FrameManager& fm) {
    apicID = apic;
    cpuID = cpu;
    frameManager = &fm;
  }
  void install() {
    MSR::write(MSR::FS_BASE, mword(this));
  }
  void initThread(Thread& t) {
    thread = idleThread = &t;
  }
  static mword getApicID() {
    mword x; asm("mov %%fs:0, %0" : "=r"(x)); return x;
  }
  static mword getCpuID() {
    mword x; asm("mov %%fs:8, %0" : "=r"(x)); return x;
  }
  static Thread* getThread() {
    mword x; asm("mov %%fs:16, %0" : "=r"(x)); return (Thread*)x;
  }
  static void setThread(Thread* x) {
             asm volatile("mov %0, %%fs:16" :: "r"(x) : "memory");
  }
  static Thread* getIdleThread() {
    mword x; asm("mov %%fs:24, %0" : "=r"(x)); return (Thread*)x;
  }
  static void setFrameManager(FrameManager* x) {
             asm volatile("mov %0, %%fs:32" :: "r"(x) : "memory");
  }
  static FrameManager* getFrameManager() {
    mword x; asm("mov %%fs:32, %0" : "=r"(x)); return (FrameManager*)x;
  }
  static mword getLockCount() {
    KASSERT(!interruptsEnabled(), "");
    mword x; asm("mov %%fs:40, %0" : "=r"(x)); return x;
  }
  static mword incLockCount() {
    KASSERT(!interruptsEnabled(), "");
    mword x; asm volatile("mov %%fs:40, %0" : "=r"(x) :: "memory");
             asm volatile("mov %0, %%fs:40" :: "r"(x+1) : "memory");
   return x+1;
  }
  static mword decLockCount() {
    KASSERT(!interruptsEnabled(), "");
    mword x; asm volatile("mov %%fs:40, %0" : "=r"(x) :: "memory");
             asm volatile("mov %0, %%fs:40" :: "r"(x-1) : "memory");
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
    KASSERT(err == 0, (ptr_t)err);
  }

  void sendStartupIPI(uint8_t vector) {
    mword err = lapic()->sendStartupIPI(apicID, vector);
    KASSERT(err == 0, (ptr_t)err);
  }

  void sendWakeupIPI() {
    mword err = lapic()->sendIPI(apicID, 0x40 );
    KASSERT(err == 0, (ptr_t)err);
  }

  void sendTestIPI() {
    mword err = lapic()->sendIPI(apicID, 0x41 );
    KASSERT(err == 0, (ptr_t)err);
  }

} __packed;

#endif /* Processor_h_ */
