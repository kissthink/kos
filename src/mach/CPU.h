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
#ifndef _CPU_h_
#define _CPU_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

class RFlags {
  static const BitSeg<mword, 0,1> CF;
  static const BitSeg<mword, 2,1> PF;
  static const BitSeg<mword, 4,1> AF;
  static const BitSeg<mword, 6,1> ZF;
  static const BitSeg<mword, 7,1> SF;
  static const BitSeg<mword, 8,1> TF;
  static const BitSeg<mword, 9,1> IF;
  static const BitSeg<mword,10,1> DF;
  static const BitSeg<mword,11,1> OF;
  static const BitSeg<mword,12,2> IOPL;
  static const BitSeg<mword,14,1> NT;
  static const BitSeg<mword,16,1> RF;
  static const BitSeg<mword,17,1> VN;
  static const BitSeg<mword,18,1> AC;
  static const BitSeg<mword,19,1> VIF;
  static const BitSeg<mword,20,1> VIP;
  static const BitSeg<mword,21,1> ID;

  // read must be volatile, since hasCPUID uses write/read pattern
  static inline mword read() {
    mword x; asm volatile("pushf\n\tpop %0" : "=r"(x) :: "memory"); return x;
  }
  static inline void write( mword x ) {
    asm volatile("push %0\n\tpopf" :: "r"(x) : "memory");
  }

public:
  RFlags() = delete;                         // no creation
  RFlags(const RFlags&) = delete;            // no copy
  RFlags& operator=(const RFlags&) = delete; // no assignment

  static bool hasCPUID() {
    mword rf = read();
    rf ^= ID();
    write(rf);
    mword rf2 = read();
    return ((rf ^ rf2) & ID()) == 0;
  }
  static bool interruptsEnabled() {
    mword rf = read();
    return rf & IF();
  }
};

namespace CPU {
  static inline mword readTSC() {
    mword a,d; asm volatile("rdtsc" : "=a"(a), "=d"(d) :: "memory"); return (d<<32)|a;
  }

  static inline mword readCR0() { // see Intel Vol 3, Section 2.5 "Control Registers"
    mword val; asm volatile("mov %%cr0, %0" : "=r"(val) :: "memory"); return val;
  }
  static inline void writeCR0( mword val ) {
    asm volatile("mov %0, %%cr0" :: "r"(val) : "memory");
  }
  static const BitSeg<mword, 0,1> PE;
  static const BitSeg<mword, 1,1> MP;
  static const BitSeg<mword, 2,1> EM;
  static const BitSeg<mword, 3,1> TS;
  static const BitSeg<mword, 4,1> ET;
  static const BitSeg<mword, 5,1> NE;
  static const BitSeg<mword,16,1> WP;
  static const BitSeg<mword,18,1> AM;
  static const BitSeg<mword,29,1> NW;
  static const BitSeg<mword,30,1> CD;
  static const BitSeg<mword,31,1> PG;

  static inline mword readCR2() {
    mword val; asm volatile("mov %%cr2, %0" : "=r"(val) :: "memory"); return val;
  }

  static inline mword readCR3() {
    mword val; asm volatile("mov %%cr3, %0" : "=r"(val) :: "memory"); return val;
  }
  static inline void writeCR3( mword val ) {
    asm volatile("mov %0, %%cr3" :: "r"(val) : "memory");
  }

  static inline mword readCR4() {
    mword val; asm volatile("mov %%cr4, %0" : "=r"(val) :: "memory"); return val;
  }
  static inline void writeCR4( mword val ) {
    asm volatile("mov %0, %%cr4" :: "r"(val) : "memory");
  }
  static const BitSeg<mword, 0,1> VME;
  static const BitSeg<mword, 1,1> PVI;
  static const BitSeg<mword, 2,1> TSD;
  static const BitSeg<mword, 3,1> DE;
  static const BitSeg<mword, 4,1> PSE;
  static const BitSeg<mword, 5,1> PAE;
  static const BitSeg<mword, 6,1> MCE;
  static const BitSeg<mword, 7,1> PGE;
  static const BitSeg<mword, 8,1> PCE;
  static const BitSeg<mword, 9,1> OSFXSR;
  static const BitSeg<mword,10,1> OSXMMEXCPT;
  static const BitSeg<mword,13,1> VMXE;
  static const BitSeg<mword,14,1> SMXE;
  static const BitSeg<mword,16,1> FSGSBASE;
  static const BitSeg<mword,17,1> PCIDE;
  static const BitSeg<mword,18,1> OSXSAVE;
  static const BitSeg<mword,20,1> SMEP;

  static inline mword readCR8() {
    mword val; asm volatile("mov %%cr8, %0" : "=r"(val) :: "memory"); return val;
  }
  static inline void writeCR8( mword val ) {
    asm volatile("mov %0, %%cr8" :: "r"(val) : "memory");
  }
  static const BitSeg<mword, 0,4> TPR; // task priority in CR8

  static void enableInterrupts() {
    asm volatile( "sti" ::: "memory" );
  }
  static void disableInterrupts() {
    asm volatile( "cli" ::: "memory" );
  }
  static void invTLB( mword val ) {
    asm volatile( "invlpg %0" :: "g"(val) : "memory" );
  }
};

struct MSR {
  static void read( uint32_t msr, uint32_t& lo, uint32_t& hi ) {
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr) : "memory");
  }
  static void write( uint32_t msr, uint32_t lo, uint32_t hi ) {
    asm volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(msr)  : "memory" );
  }
  static void read( uint32_t msr, uint64_t& val ) {
    uint32_t lo, hi;
    read(msr, lo, hi);
    val = uint64_t(hi) << 32 | lo;
  }
  static void write( uint32_t msr, uint64_t val ) {
    write(msr, val & 0xFFFFFFFF, val >> 32);
  }

  enum {
    IA32_APIC_BASE    = 0x0000001B,
    IA32_EFER         = 0xC0000080,
    IA32_TSC_DEADLINE = 0x000006E0,
    FS_BASE           = 0xC0000100,
    GS_BASE           = 0xC0000101
  };

  static const BitSeg<mword, 7,1> SYSCALL;
  static const BitSeg<mword, 8,1> BSP;
  static const BitSeg<mword,11,1> AE;
  static const BitSeg<mword,11,1> NX;

  static void enableAPIC() {
    mword apicBA;
    read( IA32_APIC_BASE, apicBA );
    apicBA |= AE();
    write( IA32_APIC_BASE, apicBA );
  }
  static bool isBSP() {
    mword apicBA;
    read( IA32_APIC_BASE, apicBA );
    return apicBA & BSP();
  }
  static void enableNX() {
    mword efer;
    read( IA32_EFER, efer );
    efer |= NX();
    write( IA32_EFER, efer );
  }
  static void enableSYSCALL() {
    mword efer;
    read( IA32_EFER, efer );
    efer |= SYSCALL();
    write( IA32_EFER, efer );
  }
};

// TODO: handle unsupported CPUID requests...
class CPUID {
  static const BitSeg<uint32_t, 2,1> ARAT;  // Always Running APIC timer (i.e. constant rate during deep sleep modes)
  static const BitSeg<uint32_t, 3,1> MWAIT;
  static const BitSeg<uint32_t, 5,1> MSR;
  static const BitSeg<uint32_t, 9,1> APIC;
  static const BitSeg<uint32_t,20,1> NX;
  static const BitSeg<uint32_t,21,1> X2APIC;
  static const BitSeg<uint32_t,24,1> TSC_Deadline;
  static const BitSeg<uint32_t,24,8> LAPIC_ID;

  static inline void cpuid( uint32_t ia, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d ) {
    asm volatile("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(ia) : "memory");
  }
  static inline void cpuid( uint32_t ia, uint32_t ic, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d ) {
    asm volatile("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(ia),"c"(ic) : "memory");
  }

public:
  CPUID() = delete;                         // no creation
  CPUID(const CPUID&) = delete;             // no copy
  CPUID& operator=(const CPUID&) = delete;  // no assignment

  static inline bool hasAPIC() {
    uint32_t a,b,c,d;
    cpuid( 0x01, a, b, c, d );
    return d & APIC();
  }
  static inline bool hasMWAIT() {
    uint32_t a,b,c,d;
    cpuid( 0x01, a, b, c, d );
    return c & MWAIT();
  }
  static inline bool hasMSR() {
    uint32_t a,b,c,d;
    cpuid( 0x01, a, b, c, d );
    return d & MSR();
  }
  static inline uint8_t getLAPIC_ID() {
    uint32_t a,b,c,d;
    cpuid( 0x01, a, b, c, d );
    return LAPIC_ID.get(b);
  }
  static inline bool hasTSC_Deadline() {
    uint32_t a,b,c,d;
    cpuid( 0x01, a, b, c, d );
    return c & TSC_Deadline();
  }
  static inline bool hasX2APIC() {
    uint32_t a,b,c,d;
    cpuid( 0x01, a, b, c, d );
    return c & X2APIC();
  }
  static inline bool hasARAT() {
    uint32_t a,b,c,d;
    cpuid( 0x06, a, b, c, d );
    return a & ARAT();
  }
  static inline bool hasNX() {
    uint32_t a,b,c,d;
    cpuid( 0x80000001, a, b, c, d );
    return d & NX();
  }
  void getCacheInfo() __section(".kernel.init");
};

#endif /* _CPU_h_ */
