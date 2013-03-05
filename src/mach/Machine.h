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
#ifndef _Machine_h_
#define _Machine_h_ 1

#include "mach/Descriptors.h"

class Device;
class Processor;

class Machine {
  static const unsigned int nullSelector     = 0; // invalid null selector
  static const unsigned int kernCodeSelector = 1; // 1nd descriptor by convention?
  static const unsigned int kernDataSelector = 2; // 2nd descriptor by convention?
  static const unsigned int userCodeSelector = 3; // 1nd descriptor by convention?
  static const unsigned int userDataSelector = 4; // 2nd descriptor by convention?
  static const unsigned int tssSelector      = 5; // uses 2 entries
  static const unsigned int maxGDT           = 7;
  static SegmentDescriptor gdt[maxGDT];

  static const unsigned int maxIDT = 256;
  static InterruptDescriptor idt[maxIDT];

  static Processor* processorTable;
  static uint32_t cpuCount;
  static uint32_t bspIndex;
  static uint32_t bspApicID;

  struct IrqInfo {
    laddr ioApicAddr;
    uint32_t ioapicIrq;
  };
  static uint32_t irqCount;
  static IrqInfo* irqTable;

  struct IrqOverrideInfo {
    uint32_t global;
    uint16_t flags;
  };
  static IrqOverrideInfo* irqOverrideTable;

  static void initDebug( vaddr mbi, bool msg )         __section(".kernel.init");
  static vaddr findModEnd( vaddr mbi )                 __section(".kernel.init");
  static void parseMBI( vaddr mbi, vaddr& rsdp )       __section(".kernel.init");
  static void probePCI()                               __section(".kernel.init");

  // code in mach/ACPI,.cc
  static void initACPI( vaddr rsdp )                   __section(".kernel.init");
  static void parseACPI()                              __section(".kernel.init");

  static void setupIDT( unsigned int number, vaddr address );
  static inline void setupGDT( unsigned int number, unsigned int dpl, uint32_t address, bool code = false );
  static inline void setupTSS( unsigned int number, uint64_t tssAddress);
  static void setupAllIDTs();                          __section(".kernel.init");

public:
  Machine() = delete;                          // no creation
  Machine(const Machine&) = delete;            // no copy
  Machine& operator=(const Machine&) = delete; // no assignment

  static void initAP(funcvoid_t)                       __section(".kernel.init");
  static void initAP2(); // cannot be in .kernel.init
  static void init(mword magic, vaddr mbi, funcvoid_t) __section(".kernel.init");
  static void init2();   // can't be in .kernel.init
  static void staticDeviceInit(Device& dev)            __section(".kernel.init");
  static void staticEnableIRQ(mword irq, mword idtnum) __section(".kernel.init");

  static inline Processor& getProcessor(uint32_t index);

  friend void exceptionHandler(int, void (*)());
};

#endif /* _Machine_h_ */
