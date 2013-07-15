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
#include "mach/CPU.h"

const BitSeg<mword, 0,1> RFlags::CF;
const BitSeg<mword, 2,1> RFlags::PF;
const BitSeg<mword, 4,1> RFlags::AF;
const BitSeg<mword, 6,1> RFlags::ZF;
const BitSeg<mword, 7,1> RFlags::SF;
const BitSeg<mword, 8,1> RFlags::TF;
const BitSeg<mword, 9,1> RFlags::IF;
const BitSeg<mword,10,1> RFlags::DF;
const BitSeg<mword,11,1> RFlags::OF;
const BitSeg<mword,12,2> RFlags::IOPL;
const BitSeg<mword,14,1> RFlags::NT;
const BitSeg<mword,16,1> RFlags::RF;
const BitSeg<mword,17,1> RFlags::VN;
const BitSeg<mword,18,1> RFlags::AC;
const BitSeg<mword,19,1> RFlags::VIF;
const BitSeg<mword,20,1> RFlags::VIP;
const BitSeg<mword,21,1> RFlags::ID;

const BitSeg<uint32_t, 2,1> CPUID::ARAT;
const BitSeg<uint32_t, 3,1> CPUID::MWAIT;
const BitSeg<uint32_t, 5,1> CPUID::MSR;
const BitSeg<uint32_t, 9,1> CPUID::APIC;
const BitSeg<uint32_t,20,1> CPUID::NX;
const BitSeg<uint32_t,21,1> CPUID::X2APIC;
const BitSeg<uint32_t,24,1> CPUID::TSC_Deadline;
const BitSeg<uint32_t,24,8> CPUID::LAPIC_ID;

const BitSeg<mword, 0,1> MSR::SYSCALL;
const BitSeg<mword, 8,1> MSR::BSP;
const BitSeg<mword,11,1> MSR::AE;
const BitSeg<mword,11,1> MSR::NX;

void CPUID::getCacheInfo() {
  // TODO: see Intel® Processor Identification and the CPUID Instruction
  // Section 5.1.3 "Cache Descriptors (Function 02h)", page 30
  // create table with tag -> li1, l1d, l2, l3, tlb... info
}
