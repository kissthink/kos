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
#ifndef _Machine_h_
#define _Machine_h_ 1

#include "mach/Descriptors.h"

class Processor;

class Machine {
  static const unsigned int nullSelector     = 0; // invalid null selector
  static const unsigned int kernCodeSelector = 1; // 1nd descriptor by convention?
  static const unsigned int kernDataSelector = 2; // 2nd descriptor by convention?
  static const unsigned int userCodeSelector = 3; // 3rd descriptor by convention?
  static const unsigned int userDataSelector = 4; // 4th descriptor by convention?
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

  // code in mach/ACPI,.cc
  static void initACPI( vaddr rsdp )                   __section(".boot.text");
  static void parseACPI()                              __section(".boot.text");

  static void setupIDT( uint32_t n, laddr addr )       __section(".boot.text");
  static void setupGDT( uint32_t n, uint32_t dpl, uint32_t addr, bool ) __section(".boot.text");
  static void setupTSS( uint32_t n, laddr addr )       __section(".boot.text");
  static void setupAllIDTs()                           __section(".boot.text");

public:
  Machine() = delete;                                  // no creation
  Machine(const Machine&) = delete;                    // no copy
  Machine& operator=(const Machine&) = delete;         // no assignment

  static void initAP(funcvoid_t)                       __section(".boot.text");
  static void initAP2();                               // not in .boot.text!
  static void initBSP(mword magic, vaddr mbi, funcvoid_t) __section(".boot.text");
  static void initBSP2();                              // not in .boot.text!
  static void staticEnableIRQ(mword irq, mword idtnum) __section(".boot.text");

  static void loadTSSRSP(uint8_t privilegeLevel,vaddr RSP);


  static inline void timerInterrupt(mword);

  friend void exceptionHandler(int, void (*)());
};

#endif /* _Machine_h_ */
