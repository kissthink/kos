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
#include "mach/Processor.h"
#include "mach/asm_functions.h"
#include "gdb/Gdb.h"
#include "kern/AddressSpace.h"
#include "kern/Thread.h"
#include "kern/Debug.h"

void Processor::checkCapabilities(bool print) {
  KASSERT0(__atomic_always_lock_free(sizeof(mword),0));
  KASSERT0(RFlags::hasCPUID()); if (print) DBG::out(DBG::Basic, " CPUID");
  KASSERT0(CPUID::hasAPIC());   if (print) DBG::out(DBG::Basic, " APIC");
  KASSERT0(CPUID::hasMSR());    if (print) DBG::out(DBG::Basic, " MSR");
  KASSERT0(CPUID::hasNX());     if (print) DBG::out(DBG::Basic, " NX");
  if (CPUID::hasMWAIT())        if (print) DBG::out(DBG::Basic, " MWAIT");
  if (CPUID::hasARAT())         if (print) DBG::out(DBG::Basic, " ARAT");
  if (CPUID::hasTSC_Deadline()) if (print) DBG::out(DBG::Basic, " TSC");
  if (CPUID::hasX2APIC())       if (print) DBG::out(DBG::Basic, " X2A");
                                if (print) DBG::out(DBG::Basic, kendl);
}

void Processor::configure() {
  checkCapabilities(false);                      // check CPU properties
  MSR::enableNX();                               // enable NX paging bit
  CPU::writeCR4(CPU::readCR4() | CPU::PGE());    // enable  G paging bit

  MSR::enableSYSCALL();                          // enable syscall/sysret
  // top  16 bits: userCodeSelector * 8 + 3 (CPL); assume DataSelector is next
  // next 16 bits: kernCodeSelector * 8 + 0 (CPL); assume DataSelector is next
  MSR::write(MSR::SYSCALL_STAR, 0x001B000800000000);
  MSR::write(MSR::SYSCALL_LSTAR, mword(syscallHandler));
  MSR::write(MSR::SYSCALL_CSTAR, 0x0);
  MSR::write(MSR::SYSCALL_SFMASK, 0x0);
}

void Processor::init(FrameManager& fm, AddressSpace& as,
  InterruptDescriptor* idtTable, size_t idtSize) {

  configure();                                   // basic setup

  MSR::write(MSR::GS_BASE, mword(this));         // store 'this' in gs

  loadIDT(idtTable, idtSize);                    // install interrupt table

  frameManager = &fm;                            // set frame manager
  as.activate();                                 // activate address space

  memset(&tss, 0, sizeof(TaskStateSegment));     // set up TSS
  vaddr stack = as.allocPages<1>(Thread::defaultStack, AddressSpace::Data);
  KASSERT0(stack != topaddr);
  tss.rsp[0] = stack + Thread::defaultStack;     // stack for CPL 0

  memset(gdt, 0, sizeof(gdt));                   // set up GDT
  setupGDT(kernCodeSelector, 0, 0, true);
  setupGDT(kernDataSelector, 0, 0, false);
  setupGDT(userCodeSelector, 3, 0, true);
  setupGDT(userDataSelector, 3, 0, false);
  setupTSS(tssSelector, (vaddr)&tss);
  loadGDT(gdt, maxGDT * sizeof(SegmentDescriptor));
  loadTR(tssSelector * sizeof(SegmentDescriptor));
  clearLDT();                                    // LDT is not used

  enableAPIC();                                  // enable APIC

  gdbCpu = Gdb::setupGdb(cpuID);                 // setup gdb

  currThread = idleThread = Thread::create(as, "idle", pagesize<1>());
}

void Processor::start(funcvoid_t func) {
  getCurrThread()->runDirect(func);              // switch to proper stack
}

void Processor::setupGDT(unsigned int number, unsigned int dpl, uint32_t address, bool code) {
  KASSERT1(number < maxGDT, number);
  KASSERT1(dpl < 4, dpl);
  gdt[number].Base00 = (address & 0x0000FFFF);
  gdt[number].Base16 = (address & 0x00FF0000) >> 16;
  gdt[number].Base24 = (address & 0xFF000000) >> 24;
  gdt[number].RW = 1;
  gdt[number].C = code ? 1 : 0;
  gdt[number].S = 1;
  gdt[number].DPL = dpl;
  gdt[number].P = 1;
  gdt[number].L = 1;
}

void Processor::setupTSS(unsigned int number, laddr address) {
  SystemDescriptor* tssDesc = (SystemDescriptor*)&gdt[number];
  tssDesc->Base00 = (address & 0x000000000000FFFF);
  tssDesc->Base16 = (address & 0x0000000000FF0000) >> 16;
  tssDesc->Type = 0x9;                                // available 64-bit TSS
  tssDesc->DPL = 3;
  tssDesc->P = 1;
  tssDesc->Base24 = (address & 0x00000000FF000000) >> 24;
  tssDesc->Base32 = (address & 0xFFFFFFFF00000000) >> 32;
}
