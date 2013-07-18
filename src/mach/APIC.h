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
#ifndef _APIC_h_
#define _APIC_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

struct PIC {
  enum IRQs { // see http://en.wikipedia.org/wiki/Intel_8259
    PIT      =  0,
    Keyboard =  1,
    Serial1  =  3,
    Serial0  =  4,
    Floppy   =  6,
    Parallel =  7,
    RTC      =  8,
    SCI      =  9,
    Mouse    = 12,
    Max      = 16
  };
  static void disable();
};

// Note that APIC registers have to be written as a complete 32-bit word
class LAPIC { // see Intel Vol 3, Section 10.4 "Local APIC"
  __aligned(0x10) uint32_t Reserved0x000;         // 0x000
  __aligned(0x10) uint32_t Reserved0x010;
  __aligned(0x10) uint32_t ID;
    const BitSeg<uint32_t,24,8> LAPIC_ID;

  __aligned(0x10) uint32_t Version;               // 0x030
  __aligned(0x10) uint32_t Reserved0x040;
  __aligned(0x10) uint32_t Reserved0x050;
  __aligned(0x10) uint32_t Reserved0x060;
  __aligned(0x10) uint32_t Reserved0x070;
  __aligned(0x10) uint32_t TaskPriority;          // 0x080
  __aligned(0x10) uint32_t ArbitrationPriority;
  __aligned(0x10) uint32_t ProcessorPriority;
  __aligned(0x10) uint32_t EOI;
  __aligned(0x10) uint32_t RemoteRead;
  __aligned(0x10) uint32_t LogicalDest;
  __aligned(0x10) uint32_t DestFormat;
  __aligned(0x10) uint32_t SpuriosInterruptVector;
    const BitSeg<uint32_t, 0,8> SpuriousVector;
    const BitSeg<uint32_t, 8,1> SoftwareEnable;
    const BitSeg<uint32_t, 9,1> FocusProcCheck;
    const BitSeg<uint32_t,12,1> BroadcastSupp;

  __aligned(0x10) uint32_t InService0;            // 0x100
  __aligned(0x10) uint32_t InService1;
  __aligned(0x10) uint32_t InService2;
  __aligned(0x10) uint32_t InService3;
  __aligned(0x10) uint32_t InService4;
  __aligned(0x10) uint32_t InService5;
  __aligned(0x10) uint32_t InService6;
  __aligned(0x10) uint32_t InService7;
  __aligned(0x10) uint32_t TriggerMode0;          // 0x180
  __aligned(0x10) uint32_t TriggerMode1;
  __aligned(0x10) uint32_t TriggerMode2;
  __aligned(0x10) uint32_t TriggerMode3;
  __aligned(0x10) uint32_t TriggerMode4;
  __aligned(0x10) uint32_t TriggerMode5;
  __aligned(0x10) uint32_t TriggerMode6;
  __aligned(0x10) uint32_t TriggerMode7;
  __aligned(0x10) uint32_t InterruptRequest0;     // 0x200
  __aligned(0x10) uint32_t InterruptRequest1;
  __aligned(0x10) uint32_t InterruptRequest2;
  __aligned(0x10) uint32_t InterruptRequest3;
  __aligned(0x10) uint32_t InterruptRequest4;
  __aligned(0x10) uint32_t InterruptRequest5;
  __aligned(0x10) uint32_t InterruptRequest6;
  __aligned(0x10) uint32_t InterruptRequest7;
  __aligned(0x10) uint32_t ErrorStatus;           // 0x280
    const BitSeg<uint32_t, 0,1> SendChecksum;
    const BitSeg<uint32_t, 1,1> ReceiveChecksum;
    const BitSeg<uint32_t, 2,1> SendAccept;
    const BitSeg<uint32_t, 3,1> ReceiveAccept;
    const BitSeg<uint32_t, 4,1> RedirectableIPI;
    const BitSeg<uint32_t, 5,1> SendIllegalVector;
    const BitSeg<uint32_t, 6,1> ReceiveIllegalVector;
    const BitSeg<uint32_t, 7,1> IllegalRegisterAddress;

  __aligned(0x10) uint32_t Reserved0x290;
  __aligned(0x10) uint32_t Reserved0x2a0;
  __aligned(0x10) uint32_t Reserved0x2b0;
  __aligned(0x10) uint32_t Reserved0x2c0;
  __aligned(0x10) uint32_t Reserved0x2d0;
  __aligned(0x10) uint32_t Reserved0x2e0;
  __aligned(0x10) uint32_t LVT_CMCI;
  __aligned(0x10) uint32_t ICR_LOW;               // 0x300
    const BitSeg<uint32_t, 0,8> Vector;
    const BitSeg<uint32_t, 8,3> DeliveryMode;     // see below
    const BitSeg<uint32_t,11,1> DestinationMode;  // 0: Physical, 1: Logical
    const BitSeg<uint32_t,12,1> DeliveryPending;  // 0: Idle, 1: Pending
    const BitSeg<uint32_t,14,1> Level;
    const BitSeg<uint32_t,15,1> TriggerMode;      // 0: Edge, 1: Level
    const BitSeg<uint32_t,18,2> DestinationShorthand;

  __aligned(0x10) uint32_t ICR_HIGH;              // 0x310
    const BitSeg<uint32_t,24,8> DestField;

  __aligned(0x10) uint32_t LVT_Timer;             // 0x320
    const BitSeg<uint32_t,16,1> MaskTimer;
    const BitSeg<uint32_t,17,2> TimerMode;

  __aligned(0x10) uint32_t LVT_ThermalSensor;     // 0x330
  __aligned(0x10) uint32_t LVT_PMCs;
  __aligned(0x10) uint32_t LVT_LINT0;
  __aligned(0x10) uint32_t LVT_LINT1;
  __aligned(0x10) uint32_t LVT_Error;
  __aligned(0x10) uint32_t InitialCount;          // 0x380
  __aligned(0x10) uint32_t CurrentCount;
  __aligned(0x10) uint32_t Reserved0x3a0;
  __aligned(0x10) uint32_t Reserved0x3b0;
  __aligned(0x10) uint32_t Reserved0x3c0;
  __aligned(0x10) uint32_t Reserved0x3d0;
  __aligned(0x10) uint32_t DivideConfiguration;
  __aligned(0x10) uint32_t Reserved0x3f0;

  enum DeliveryModes { // Intel Vol. 3, Section 10.6.1 "Interrupt Command Register"
    Fixed          = 0b000,
    LowestPriority = 0b001,
    SMI            = 0b010,
    NMI            = 0b100,
    Init           = 0b101,
    Startup        = 0b110
  };

  enum DestinationShorthands { // Intel Vol. 3, Section 10.6.1 "Interrupt Command Register"
    None        = 0b00,
    Self        = 0b01,
    AllInclSelf = 0b10,
    AllExclSelf = 0b11
  };

  enum TimerModes { //
    OneShot  = 0b00,
    Periodic = 0b01,
    Deadline = 0b10
  };

public:
  LAPIC() = delete;                              // no creation
  LAPIC(const LAPIC&) = delete;                  // no copy
  const LAPIC& operator=(const LAPIC&) = delete; // no assignment

  void enable(uint8_t sv) volatile {
    SpuriosInterruptVector |= SpuriousVector(sv) | SoftwareEnable();
  }
  void disable() volatile {
    SpuriosInterruptVector &= ~SoftwareEnable();
  }
  void setFlatMode() volatile {
    DestFormat = 0xFFFFFFFF;
  }
  void maskTimer() volatile {
    LVT_Timer |= MaskTimer();
  }
  void maskInterrupts() volatile {
    LVT_LINT0 |= MaskTimer();
    LVT_LINT1 |= MaskTimer();
  }
  void unMaskInterrupts() volatile {
    LVT_LINT0 &= ~MaskTimer();
    LVT_LINT1 &= ~MaskTimer();
  }
  uint8_t getLAPIC_ID() volatile {
    return LAPIC_ID.get(ID);
  }
  void sendEOI() volatile {
    EOI = 0;
  }
  uint32_t sendInitIPI( uint8_t dest ) volatile {
    ICR_HIGH = DestField(dest);
    ICR_LOW = DeliveryMode(Init) | Level();
    while (ICR_LOW & DeliveryPending());
    return ErrorStatus;
  }
  uint32_t sendStartupIPI( uint8_t dest, uint8_t vec ) volatile {
    ICR_HIGH = DestField(dest);
    ICR_LOW = DeliveryMode(Startup) | Level() | Vector(vec);
    while (ICR_LOW & DeliveryPending());
    return ErrorStatus;
  }
  uint32_t sendIPI( uint8_t dest, uint8_t vec ) volatile {
    ICR_HIGH = DestField(dest);
    ICR_LOW = DeliveryMode(Fixed) | Level() | Vector(vec);
//    while (ICR_LOW & DeliveryPending());
    return ErrorStatus;
  }
} __packed;

class IOAPIC { // Intel 82093AA IOAPIC, Section 3.1 "Memory Mapped Registers..."
  __aligned(0x10) uint32_t RegisterSelect;        // 0x000
  __aligned(0x10) uint32_t Window;
  void write( uint8_t reg, uint32_t val ) volatile {
    RegisterSelect = reg;
    Window = val;
  }
  uint32_t read( uint8_t reg ) volatile {
    RegisterSelect = reg;
    return Window;
  }
  enum Registers : uint8_t {
    IOAPICID =  0x00,
    IOAPICVER = 0x01,
    IOAPICARB = 0x02,
    IOREDTBL =  0x10
  };
  const BitSeg<uint32_t, 0, 8> Version;
  const BitSeg<uint32_t,16, 8> MaxRedirectionEntry;
  const BitSeg<uint32_t,24, 4> ID;
  const BitSeg<uint32_t,24, 4> ArbitrationID;

  const BitSeg<uint64_t, 0, 8> Vector;          // interrupt number
  const BitSeg<uint64_t, 8, 3> DeliveryMode;    // see Intel spec, cf. APIC
  const BitSeg<uint64_t,11, 1> DestinationMode; // 0: Physical, 1: Logical
  const BitSeg<uint64_t,12, 1> DeliveryPending; // 0: Idle, 1: Pending
  const BitSeg<uint64_t,13, 1> Polarity;        // 0: High, 1: Low
  const BitSeg<uint64_t,14, 1> RemoteIRR;       // 0: EOI received
  const BitSeg<uint64_t,15, 1> TriggerMode;     // 0: Edge, 1: Level
  const BitSeg<uint64_t,16, 1> Mask;            // 1: interrupt masked
  const BitSeg<uint64_t,56, 4> DestinationID;   // APIC ID
  const BitSeg<uint64_t,56, 8> DestinationSet;  // group, cf APIC spec
  
public:
  IOAPIC() = delete;                               // no creation
  IOAPIC(const IOAPIC&) = delete;                  // no copy
  const IOAPIC& operator=(const IOAPIC&) = delete; // no assignment

  uint8_t getVersion() volatile;
  uint8_t getRedirects() volatile;
  void maskIRQ(uint8_t irq) volatile;
  void mapIRQ(uint8_t irq, uint8_t intr, uint32_t apicID) volatile;
} __packed;

#endif /* APIC */
