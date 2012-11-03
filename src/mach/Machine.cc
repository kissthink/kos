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
#include "extern/multiboot/multiboot2.h"
#include "mach/Machine.h"
#include "mach/PageManager.h"
#include "mach/PCI.h"
#include "mach/asm_functions.h"
#include "mach/isr_wrapper.h"
#include "dev/Keyboard.h"
#include "dev/RTC.h"
#include "dev/Screen.h"
#include "kern/AddressSpace.h"
#include "kern/FrameManager.h"
#include "kern/KernelHeap.h"
#include "kern/PCQ.h"
#include "kern/Scheduler.h"
#include "kern/Thread.h"

#include <list>

// symbols set during linking, see linker.ld
extern const char __ctors_start, __ctors_end;
extern const char __KernelBoot, __KernelBootEnd;
extern const char __KernelCode, __KernelCodeEnd;
extern const char __KernelRO,   __KernelRO_End;
extern const char __KernelData, __KernelDataEnd;
extern const char __KernelBss,  __KernelBssEnd;
extern const char _loadStart, _loadEnd, _bssEnd; // used for multiboot, just checking here

// symbols from boot.asm that point to 16-bit boot code location
extern const char boot16Begin, boot16End;

// symbols from Screen.h that need to be defined
char Screen::buffer[xmax * ymax];
char* Screen::video = nullptr;

// descriptor tables
GateDescriptor    Machine::idt[Machine::maxIDT] __aligned(0x1000);
SegmentDescriptor Machine::gdt[Machine::maxGDT] __aligned(0x1000);

// static global object
static FrameManager frameManager;

// processor/core information
Processor* Machine::processorTable = nullptr;
uint32_t Machine::cpuCount = 0;
uint32_t Machine::bspIndex = ~0;
uint32_t Machine::bspApicID = ~0;

// used to enumerate APs during bootstrap
static mword apIndex = mwordlimit;

// IRQ information
uint32_t Machine::irqCount = 0;
Machine::IrqInfo* Machine::irqTable = nullptr;
Machine::IrqOverrideInfo* Machine::irqOverrideTable = nullptr;

// list of devices that are initialized statically during bootstrap
static std::list<Device*,KernelAllocator<Device*>> deviceInitList;

// PCQ between keyboard and rest of system
static PCQ<uint16_t> keycodeQueue(128);

// static keyboard and RTC device object
static Keyboard keyboard(keycodeQueue);
static RTC rtc;

// check various assumptions about data types and sizes
static_assert(mword(true) == mword(1), "true == mword(1)");
static_assert(__atomic_always_lock_free(sizeof(mword),0), "mword not lock-free");
static_assert(sizeof(uint64_t) == sizeof(mword), "mword == uint64_t" );
static_assert(sizeof(size_t) == sizeof(mword), "mword == size_t");
static_assert(sizeof(ptr_t) == sizeof(mword), "mword == ptr_t");
static_assert(sizeof(LAPIC) == 0x400, "sizeof(LAPIC) == 0x400" );
static_assert(sizeof(GateDescriptor) == 2 * sizeof(mword), "sizeof(GateDescriptor) == 2 * sizeof(mword)" );
static_assert(sizeof(SegmentDescriptor) == sizeof(mword), "sizeof(SegmentDescriptor) == sizeof(mword)" );

// simple thread to print keycode on screen
static void screenThread(ptr_t pcq_p) {
  PCQ<uint16_t>& kcq = *(PCQ<uint16_t>*)pcq_p;
  for (;;) {
    kcerr << " kc:" << (ptr_t)mword(kcq.remove());
  }
}

// main init routine for APs
void Machine::initAP(funcvoid_t func) {
  // setup NX, GDT, address space, IDT
  MSR::enableNX();
  loadGDT(gdt, maxGDT * sizeof(SegmentDescriptor));
  kernelSpace.activate();
  loadIDT(idt, sizeof(idt));

  // configure local processor
  processorTable[apIndex].install();
  Thread* idleThread = Thread::create(pagesize<1>());
  processorTable[apIndex].initThread(*idleThread);

  // print brief message
  DBG::out(DBG::Basic, ' ', apIndex);

  // leave init stack and start idle loop -> will call initAP2
  idleThread->runDirect(func);
}

// 2nd init routine for APs - on new stack, processor object initialized
void Machine::initAP2() {
  // enable APIC
  MSR::enableAPIC();                // should be enabled by default
  Processor::enableAPIC(0xf8);      // confirm spurious vector at 0xf8

  // enable interrupts, sync with BSP, then halt
  processorTable[apIndex].startInterrupts();
  MemoryBarrier();                  // make sure everything above is completed
  apIndex = bspIndex;
  Halt();
}

// main init routine for BSP
void Machine::init(mword magic, vaddr mbiAddr, funcvoid_t func) {
  // initialize bss
  memset((char*)&__KernelBss, 0, &__KernelBssEnd - &__KernelBss);

  // initialize debugging: no debug output before this point!
  initDebug(mbiAddr,false);

  // dummy processor to manage interrupts: no locking before this point!
  Processor dummy;
  dummy.install();

  // bootstrap kernel heap with pre-allocated memory -> uses locking
  vaddr mbiEnd = align_up(mbiAddr + *(uint32_t*)mbiAddr, pagesize<1>());
  vaddr kernelEnd = KERNBASE + BOOTMEM;
  KernelHeap::init(mbiEnd, kernelEnd);

  // initialize basic video output: no screen output before this point
  laddr videoMemory = Screen::init(kernelBase);
  if (videoMemory == 0) Reboot();

  // call global constructors: %rbx is callee-saved, thus safe to use
  // some dynamoc memory is fine, as long as it fits in pre-allocation
  for ( const char* x = &__ctors_start; x != &__ctors_end; x += sizeof(char*)) {
    asm volatile( "call *(%0)" : : "b"(x) : "memory" );
  }

  // initialize debugging again, but this time with error messages
  initDebug(mbiAddr,true);

  // make frameManager globally accessible
  dummy.setFrameManager(&frameManager);

  // install GDT (replacing boot loader's GDT)
  memset(gdt, 0, sizeof(gdt));
  setupGDT( codeSelector, 0, true );
  setupGDT( dataSelector, 0, false );
  setupGDT( fsSelector, 0, false );
  loadGDT( gdt, maxGDT * sizeof(SegmentDescriptor) );

  // install all IDT entries
  setupAllIDTs();
  loadIDT(idt, sizeof(idt));

  // check CPU capabilities
  DBG::out(DBG::Basic, "checking capabilities:");
  KASSERT(RFlags::hasCPUID(), "");
  DBG::out(DBG::Basic, " CPUID");
  KASSERT(CPUID::hasAPIC(), "");
  DBG::out(DBG::Basic, " APIC");
  KASSERT(CPUID::hasMSR(), "");
  DBG::out(DBG::Basic, " MSR");
  KASSERT(CPUID::hasNX(), "");
  DBG::out(DBG::Basic, " NX");
  if (CPUID::hasMWAIT()) DBG::out(DBG::Basic, " MWAIT");
  if (CPUID::hasARAT()) DBG::out(DBG::Basic, " ARAT");
  if (CPUID::hasTSC_Deadline()) DBG::out(DBG::Basic, " TSC");
  if (CPUID::hasX2APIC()) DBG::out(DBG::Basic, " X2A");
  DBG::out(DBG::Basic, kendl);

  // print boot memory information
  DBG::outln(DBG::Basic, "MB info:     ", (ptr_t)&_loadStart, ',', (ptr_t)&_loadEnd, ',', (ptr_t)&_bssEnd);
  DBG::outln(DBG::Basic, "Kernel Boot: ", (ptr_t)&__KernelBoot, '-', (ptr_t)&__KernelBootEnd);
  DBG::outln(DBG::Basic, "Kernel Code: ", (ptr_t)&__KernelCode, '-', (ptr_t)&__KernelCodeEnd);
  DBG::outln(DBG::Basic, "Kernel RO:   ", (ptr_t)&__KernelRO  , '-', (ptr_t)&__KernelRO_End);
  DBG::outln(DBG::Basic, "Kernel Data: ", (ptr_t)&__KernelData, '-', (ptr_t)&__KernelDataEnd);
  DBG::outln(DBG::Basic, "Kernel Bss:  ", (ptr_t)&__KernelBss , '-', (ptr_t)&__KernelBssEnd);
  DBG::outln(DBG::Basic, "boot16:      ", (ptr_t)&boot16Begin , '-', (ptr_t)&boot16End);

  // parse multiboot information -> stores memory info in frameManager
  KASSERT(magic == MULTIBOOT2_BOOTLOADER_MAGIC, magic);
  laddr acpiRSDP = 0;
  parseMBI( mbiAddr, acpiRSDP );

  // initialize frame manager with initial allocation for kernel
  DBG::outln(DBG::Basic, "FM/init: ", frameManager);
  vaddr kernelBoot = vaddr(&__KernelBoot);
  bool check = frameManager.alloc( kernelBoot - kernelBase, kernelEnd - kernelBoot );
  KASSERT( check, "cannot allocate static kernel memory" );
  DBG::outln(DBG::Basic, "FM/alloc: ", frameManager);

  // prepare boot code for APs
  check = frameManager.alloc( BOOT16, pagesize<1>() );
  KASSERT( check, "cannot allocate AP boot memory" );
  memcpy((ptr_t)BOOT16, &boot16Begin, &boot16End - &boot16Begin);
  DBG::outln(DBG::Basic, "FM/boot16: ", frameManager);

  // bootstrap PageManager, RO data is part of code segment for now
  MSR::enableNX();
  vaddr kernelRO_End = align_up(vaddr(&__KernelRO_End), pagesize<2>());
  mword kernelCodePages = (kernelRO_End - kernelBase) >> pagesizebits<2>();
  mword kernelDataPages = (kernelEnd  - kernelBase) >> pagesizebits<2>();
  laddr pml4addr = PageManager::bootstrap( frameManager, kernelBase, kernelCodePages, pagesize<2>(), kernelDataPages, pagesize<2>(), kernelBoot - kernelBase );
  DBG::outln(DBG::Basic, "FM/paging: ", frameManager);

  // initial kernel address space -> no identity mapping afterwards
  kernelSpace.init(pml4addr, kernelLow, kernelBase);
  kernelSpace.activate();

  // remap screen's virtual address, before kernel boot memory disappears
  PageManager::map<1>(videoAddr, videoMemory, PageManager::Kernel, PageManager::Data, true);
  Screen::remap(videoAddr);

  // parse ACPI tables: find/initialize CPUs, APICs, IOAPICs, static devices
  initACPI(acpiRSDP);

  // set up static devices
  for (Device* dev : deviceInitList) dev->init();
  deviceInitList.clear();

  // create main thread and leave init stack -> func will call init2
  Thread* mainThread;
  processorTable[bspIndex].install();
  mainThread = Thread::create();
  processorTable[bspIndex].initThread(*mainThread);
  mainThread->runDirect(func);
}

// 2nd init routine for BSP - on new stack, proper processor object
void Machine::init2() {
  // find additional devices, need "current thread" for ACPI subsystem init
  parseACPI();

  // find PCI devices
  probePCI();

  // enable interrupts, needed for rtc.wait below
  processorTable[bspIndex].startInterrupts();

  // send test IPI to self
  processorTable[bspIndex].sendTestIPI();

  // start up APs -> serialized, APs go into long mode and then halt
  DBG::out(DBG::Basic, "AP init:");
  for ( uint32_t i = 0; i < cpuCount; i += 1 ) {
    if ( i != bspIndex ) {
      apIndex = i;                          // prepare sync variable
      MemoryBarrier();	                    // sync memory before starting AP
      processorTable[i].sendInitIPI();
      rtc.wait(5);                          // wait for HW init
      processorTable[i].sendStartupIPI(BOOT16 / 0x1000);
      while (apIndex != bspIndex) Pause();  // wait for SW init - see initAP()
    }
  }
  DBG::out(DBG::Basic, kendl);

  // free kernel boot memory
  vaddr kernelBoot = vaddr(&__KernelBoot);
  vaddr kernelCode = align_down(vaddr(&__KernelCode), pagesize<2>());
  for ( vaddr x = kernelBase; x < kernelCode; x += pagesize<2>() ) {
    PageManager::unmap<2>(x, true);
  }
  bool check = frameManager.release(kernelBoot - kernelBase, kernelCode - kernelBoot);
  KASSERT( check, "cannot free kernel boot memory" );
  DBG::outln(DBG::Basic, "FM/free: ", frameManager);

  // free AP boot code
  check = frameManager.release(BOOT16, pagesize<1>());
  KASSERT( check, "cannot free AP boot memory" );
  DBG::outln(DBG::Basic, "FM/free16: ", frameManager);

  // wake up APs
  for ( uint32_t i = 0; i < cpuCount; i += 1 ) {
    if ( i != bspIndex ) processorTable[i].sendWakeupIPI();
  }

  // create simple keyboard-to-screen thread
  Thread::create(screenThread, &keycodeQueue);
}

void Machine::staticDeviceInit(Device& dev) {
  deviceInitList.push_back(&dev);
}

void Machine::staticEnableIRQ( mword irqnum, mword idtnum ) {
  KASSERT( irqnum < PIC::Max, irqnum );
  irqnum = irqOverrideTable[irqnum].global;
  // TODO: handle irq override flags
  kernelSpace.mapPages<1>(ioapicAddr, irqTable[irqnum].ioApicAddr, pagesize<1>(), true);
  volatile IOAPIC* ioapic = (IOAPIC*)ioapicAddr;
  ioapic->mapIRQ( irqTable[irqnum].ioapicIrq, idtnum, bspApicID );
  kernelSpace.unmapPages<1>(ioapicAddr, pagesize<1>(), true);
}

inline Processor& Machine::getProcessor(uint32_t idx) {
  return processorTable[idx];
}

void Machine::initDebug( vaddr mbi, bool msg ) {
  vaddr mbiEnd = mbi + *(uint32_t*)mbi;
  mbi += sizeof(multiboot_header_tag);
  for (;;) {
    multiboot_tag* tag = (multiboot_tag*)mbi;
  if (tag->type == MULTIBOOT_TAG_TYPE_END || mbi >= mbiEnd) break;
    if (tag->type == MULTIBOOT_TAG_TYPE_CMDLINE) {
      DBG::init( ((multiboot_tag_string*)tag)->string, msg );
  return;
    }
    mbi += (tag->size + 7) & ~7;
  }
}

void Machine::parseMBI( vaddr mbi, vaddr& rsdp ) {
  KASSERT( !(mbi & 7), mbi );
//  mword mbiEnd = mbi + ((multiboot_header_tag*)mbi)->size;
  vaddr mbiEnd = mbi + *(uint32_t*)mbi;
  DBG::outln(DBG::Boot, "MBI size: ", *(uint32_t*)mbi);
  mbi += sizeof(multiboot_header_tag);
  for (;;) {
    multiboot_tag* tag = (multiboot_tag*)mbi;
  if (tag->type == MULTIBOOT_TAG_TYPE_END || mbi >= mbiEnd) break;
    switch (tag->type) {
    case MULTIBOOT_TAG_TYPE_CMDLINE:
      DBG::outln(DBG::Boot, "command line: ", ((multiboot_tag_string*)tag)->string);
      break;
    case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
      DBG::outln(DBG::Boot, "boot loader: ", ((multiboot_tag_string*)tag)->string);
      break;
    case MULTIBOOT_TAG_TYPE_MODULE: {
      multiboot_tag_module* tm = (multiboot_tag_module*)tag;
      DBG::outln(DBG::Boot, "module at ", (ptr_t)uintptr_t(tm->mod_start),
        '-', (ptr_t)uintptr_t(tm->mod_end), ": ", tm->cmdline);
    } break;
    case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
      multiboot_tag_basic_meminfo* tm = (multiboot_tag_basic_meminfo*)tag;
      DBG::outln(DBG::Boot, "memory low: ", tm->mem_lower, " / high: ", tm->mem_upper);
    } break;
    case MULTIBOOT_TAG_TYPE_BOOTDEV: {
      multiboot_tag_bootdev* tb = (multiboot_tag_bootdev*)tag;
      DBG::outln(DBG::Boot, "boot device: ", (ptr_t)uintptr_t(tb->biosdev), ',', tb->slice, ',', tb->part);
    } break;
    case MULTIBOOT_TAG_TYPE_MMAP: {
      multiboot_tag_mmap* tmm = (multiboot_tag_mmap*)tag;
      vaddr end = mbi + tmm->size;
      DBG::out(DBG::Boot, "mmap:");
      for (mword mm = (mword)tmm->entries; mm < end; mm += tmm->entry_size ) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mm;
        DBG::out(DBG::Boot, ' ', (ptr_t)mmap->addr, '/', (ptr_t)mmap->len, '/');
        switch (mmap->type) {
          case MULTIBOOT_MEMORY_AVAILABLE: DBG::out(DBG::Boot, "free"); break;
          case MULTIBOOT_MEMORY_RESERVED:  DBG::out(DBG::Boot, "resv"); break;
          case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: DBG::out(DBG::Boot, "acpi"); break;
          case MULTIBOOT_MEMORY_NVS: DBG::out(DBG::Boot, "nvs"); break;
          case MULTIBOOT_MEMORY_BADRAM: DBG::out(DBG::Boot, "bad"); break;
          default: DBG::out(DBG::Boot, "unknown");
        }
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
          vaddr addr = align_up( vaddr(mmap->addr), pagesize<1>() );
          mword len = align_down( mword(mmap->len - (addr - mmap->addr)), pagesize<1>() );
          if ( len > 0 ) {
            DBG::out(DBG::Boot, kendl);
            frameManager.release( addr, len );
          }
        }
      }
      DBG::out(DBG::Boot, kendl);
    } break;
    case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
      DBG::outln(DBG::Boot, "framebuffer info present");
      break;
    case MULTIBOOT_TAG_TYPE_VBE:
      DBG::outln(DBG::Boot, "vbe info present");
      break;
    case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
      DBG::outln(DBG::Boot, "elf section info present");
      break;
    case MULTIBOOT_TAG_TYPE_APM:
      DBG::outln(DBG::Boot, "APM info present");
      break;
    case MULTIBOOT_TAG_TYPE_EFI32:
      DBG::outln(DBG::Boot, "efi32 info present");
      break;
    case MULTIBOOT_TAG_TYPE_EFI64:
      DBG::outln(DBG::Boot, "efi64 info present");
      break;
    case MULTIBOOT_TAG_TYPE_SMBIOS:
      DBG::outln(DBG::Boot, "smbios info present");
      break;
    case MULTIBOOT_TAG_TYPE_ACPI_OLD:
      KASSERT( rsdp == 0, rsdp );
      rsdp = vaddr(((multiboot_tag_old_acpi*)tag)->rsdp) - kernelBase;
      DBG::outln(DBG::Boot, "acpi/old: ", (ptr_t)rsdp, '/', ((multiboot_tag_old_acpi*)tag)->size);
      break;
    case MULTIBOOT_TAG_TYPE_ACPI_NEW:
      KASSERT( rsdp == 0, rsdp );
      rsdp = vaddr(((multiboot_tag_new_acpi*)tag)->rsdp) - kernelBase;
      DBG::outln(DBG::Boot, "acpi/new: ", (ptr_t)rsdp, '/', ((multiboot_tag_new_acpi*)tag)->size);
      break;
    case MULTIBOOT_TAG_TYPE_NETWORK:
      DBG::outln(DBG::Boot, "network info present");
      break;
    default:
      DBG::outln(DBG::Boot, "unknown tag: ", tag->type);
    }
    mbi += (tag->size + 7) & ~7;
  }
}

void Machine::probePCI() {
  for (int bus = 0; bus < PCI::MaxBus; bus += 1) {
    for (int dev = 0; dev < PCI::MaxDevice; dev += 1) {
      uint32_t r = PCI::probeDevice(bus,dev);
      if (PCI::Vendor.get(r) != 0xFFFF) {
        DBG::outln(DBG::PCI, "PCI ", bus, '/', dev, ' ', FmtPtr(PCI::Vendor.get(r),4), ':', FmtPtr(PCI::Device.get(r),4));
      }
    }
  }
}

extern "C" void isr_handler_0x08() { // double fault
  kcerr << "\nDOUBLE FAULT";
  Reboot( 1 );
}

extern "C" void isr_handler_0x0c(mword errcode, vaddr iAddr) { // stack fault
  kcerr << "\nSTACK FAULT - ia: " << (ptr_t)iAddr << " / flags: " << (ptr_t)errcode;
  Reboot( iAddr );
}

extern "C" void isr_handler_0x0d(mword errcode, vaddr iAddr) { // stack fault
  kcerr << "\nGENERAL PROTECTION FAULT - ia: " << (ptr_t)iAddr << " / flags: " << (ptr_t)errcode;
  Reboot( iAddr );
}

extern "C" void isr_handler_0x0e(mword errcode, vaddr iAddr) { // page fault
  kcerr << "\nPAGE FAULT - ia: " << (ptr_t)iAddr << " / da: " << (ptr_t)CPU::readCR2() << " / flags: " << (ptr_t)errcode;
  Reboot( iAddr );
}

extern "C" void isr_handler_0x20() { // PIT interrupt
  Processor::sendEOI();
}

extern "C" void isr_handler_0x21() { // keyboard interrupt
  keyboard.staticInterruptHandler(); // low-level processing *before* EOI
  Processor::sendEOI();
}

extern "C" void isr_handler_0x28() { // RTC interrupt
  rtc.staticInterruptHandler();      // low-level processing *before* EOI
  Processor::sendEOI();              // EOI *before* switching stacks
  if (rtc.tick() % 20 == 0) kernelScheduler.preempt();
  else if (rtc.tick() % 10 == 0) Machine::getProcessor(1).sendWakeupIPI();
}

extern "C" void isr_handler_0x40() {
  Processor::sendEOI();              // EOI *before* switching stacks
  kernelScheduler.preempt();
}

extern "C" void isr_handler_0x41() {
  Processor::sendEOI();
  kcerr << " TIPI";
  kcerr << (RFlags::interruptsEnabled() ? 'e' : 'd');
}

extern "C" void isr_handler_0xf8() {
  kcerr << " spurious";
}

extern "C" void isr_handler_0xff() {
  kcerr << " 0xFF";
}

extern "C" void isr_handler_gen(mword num) {
  kcerr << "\nUNEXPECTED INTERRUPT: " << (ptr_t)num << '/' << (ptr_t)CPU::readCR2() << '\n';
  Reboot();
}

extern "C" void isr_handler_gen_err(mword num, mword errcode) {
  kcerr << "\nUNEXPECTED INTERRUPT: " << (ptr_t)num << '/' << (ptr_t)CPU::readCR2() << '/' << (ptr_t)errcode << '\n';
  Reboot();
}

inline void Machine::setupIDT( unsigned int number, vaddr address ) {
  KASSERT(number < maxIDT, number);
  idt[number].Offset00 = (address & 0x000000000000FFFF);
  idt[number].Offset16 = (address & 0x00000000FFFF0000) >> 16;
  idt[number].Offset32 = (address & 0xFFFFFFFF00000000) >> 32;
  idt[number].SegmentSelector = codeSelector * sizeof(SegmentDescriptor);
  idt[number].Type = 0x0E; // 64-bit interrupt gate (trap does not disable interrupts)
  idt[number].P = 1;
}

inline void Machine::setupGDT( unsigned int number, uint32_t address, bool code ) {
  KASSERT(number < maxGDT, number);
  gdt[number].Base00 = (address & 0x0000FFFF);
  gdt[number].Base16 = (address & 0x00FF0000) >> 16;
  gdt[number].Base24 = (address & 0xFF000000) >> 24;
  gdt[number].RW = 1;
  gdt[number].C = code ? 1 : 0;
  gdt[number].S = 1;
  gdt[number].P = 1;
  gdt[number].L = 1;
}

void Machine::setupAllIDTs() {
 	KASSERT(!RFlags::interruptsEnabled(), "");
  memset(idt, 0, sizeof(idt));
  setupIDT(0x00, (vaddr)&isr_wrapper_0x00);
  setupIDT(0x01, (vaddr)&isr_wrapper_0x01);
  setupIDT(0x02, (vaddr)&isr_wrapper_0x02);
  setupIDT(0x03, (vaddr)&isr_wrapper_0x03);
  setupIDT(0x04, (vaddr)&isr_wrapper_0x04);
  setupIDT(0x05, (vaddr)&isr_wrapper_0x05);
  setupIDT(0x06, (vaddr)&isr_wrapper_0x06);
  setupIDT(0x07, (vaddr)&isr_wrapper_0x07);
 	setupIDT(0x08, (vaddr)&isr_wrapper_0x08); // double fault
 	setupIDT(0x09, (vaddr)&isr_wrapper_0x09);
 	setupIDT(0x0a, (vaddr)&isr_wrapper_0x0a);
 	setupIDT(0x0b, (vaddr)&isr_wrapper_0x0b);
	setupIDT(0x0c, (vaddr)&isr_wrapper_0x0c); // stack fault
	setupIDT(0x0d, (vaddr)&isr_wrapper_0x0d); // general protection fault
 	setupIDT(0x0e, (vaddr)&isr_wrapper_0x0e); // page fault
 	setupIDT(0x0f, (vaddr)&isr_wrapper_0x0f);
 	setupIDT(0x10, (vaddr)&isr_wrapper_0x10);
 	setupIDT(0x11, (vaddr)&isr_wrapper_0x11);
 	setupIDT(0x12, (vaddr)&isr_wrapper_0x12);
 	setupIDT(0x13, (vaddr)&isr_wrapper_0x13);
 	setupIDT(0x14, (vaddr)&isr_wrapper_0x14);
 	setupIDT(0x15, (vaddr)&isr_wrapper_0x15);
 	setupIDT(0x16, (vaddr)&isr_wrapper_0x16);
 	setupIDT(0x17, (vaddr)&isr_wrapper_0x17);
 	setupIDT(0x18, (vaddr)&isr_wrapper_0x18);
 	setupIDT(0x19, (vaddr)&isr_wrapper_0x19);
 	setupIDT(0x1a, (vaddr)&isr_wrapper_0x1a);
 	setupIDT(0x1b, (vaddr)&isr_wrapper_0x1b);
 	setupIDT(0x1c, (vaddr)&isr_wrapper_0x1c);
 	setupIDT(0x1d, (vaddr)&isr_wrapper_0x1d);
 	setupIDT(0x1e, (vaddr)&isr_wrapper_0x1e);
 	setupIDT(0x1f, (vaddr)&isr_wrapper_0x1f);
 	setupIDT(0x20, (vaddr)&isr_wrapper_0x20); // PIT interrupt
 	setupIDT(0x21, (vaddr)&isr_wrapper_0x21); // keyboard interrupt
 	setupIDT(0x22, (vaddr)&isr_wrapper_0x22);
 	setupIDT(0x23, (vaddr)&isr_wrapper_0x23);
 	setupIDT(0x24, (vaddr)&isr_wrapper_0x24);
 	setupIDT(0x25, (vaddr)&isr_wrapper_0x25);
 	setupIDT(0x26, (vaddr)&isr_wrapper_0x26);
 	setupIDT(0x27, (vaddr)&isr_wrapper_0x27);
 	setupIDT(0x28, (vaddr)&isr_wrapper_0x28); // RTC interrupt
 	setupIDT(0x29, (vaddr)&isr_wrapper_0x29); // SCI interrupt (used by ACPI)
 	setupIDT(0x2a, (vaddr)&isr_wrapper_0x2a);
 	setupIDT(0x2b, (vaddr)&isr_wrapper_0x2b);
 	setupIDT(0x2c, (vaddr)&isr_wrapper_0x2c);
 	setupIDT(0x2d, (vaddr)&isr_wrapper_0x2d);
 	setupIDT(0x2e, (vaddr)&isr_wrapper_0x2e);
 	setupIDT(0x2f, (vaddr)&isr_wrapper_0x2f);
 	setupIDT(0x30, (vaddr)&isr_wrapper_0x30);
 	setupIDT(0x31, (vaddr)&isr_wrapper_0x31);
 	setupIDT(0x32, (vaddr)&isr_wrapper_0x32);
 	setupIDT(0x33, (vaddr)&isr_wrapper_0x33);
 	setupIDT(0x34, (vaddr)&isr_wrapper_0x34);
 	setupIDT(0x35, (vaddr)&isr_wrapper_0x35);
 	setupIDT(0x36, (vaddr)&isr_wrapper_0x36);
 	setupIDT(0x37, (vaddr)&isr_wrapper_0x37);
 	setupIDT(0x38, (vaddr)&isr_wrapper_0x38);
 	setupIDT(0x39, (vaddr)&isr_wrapper_0x39);
 	setupIDT(0x3a, (vaddr)&isr_wrapper_0x3a);
 	setupIDT(0x3b, (vaddr)&isr_wrapper_0x3b);
 	setupIDT(0x3c, (vaddr)&isr_wrapper_0x3c);
 	setupIDT(0x3d, (vaddr)&isr_wrapper_0x3d);
 	setupIDT(0x3e, (vaddr)&isr_wrapper_0x3e);
 	setupIDT(0x3f, (vaddr)&isr_wrapper_0x3f);
	setupIDT(0x40, (vaddr)&isr_wrapper_0x40); // wakeup IPI
 	setupIDT(0x41, (vaddr)&isr_wrapper_0x41); // test IPI
  setupIDT(0x42, (vaddr)&isr_wrapper_0x42);
  setupIDT(0x43, (vaddr)&isr_wrapper_0x43);
  setupIDT(0x44, (vaddr)&isr_wrapper_0x44);
  setupIDT(0x45, (vaddr)&isr_wrapper_0x45);
  setupIDT(0x46, (vaddr)&isr_wrapper_0x46);
  setupIDT(0x47, (vaddr)&isr_wrapper_0x47);
  setupIDT(0x48, (vaddr)&isr_wrapper_0x48);
  setupIDT(0x49, (vaddr)&isr_wrapper_0x49);
  setupIDT(0x4a, (vaddr)&isr_wrapper_0x4a);
  setupIDT(0x4b, (vaddr)&isr_wrapper_0x4b);
  setupIDT(0x4c, (vaddr)&isr_wrapper_0x4c);
  setupIDT(0x4d, (vaddr)&isr_wrapper_0x4d);
  setupIDT(0x4e, (vaddr)&isr_wrapper_0x4e);
  setupIDT(0x4f, (vaddr)&isr_wrapper_0x4f);
  setupIDT(0x50, (vaddr)&isr_wrapper_0x50);
  setupIDT(0x51, (vaddr)&isr_wrapper_0x51);
  setupIDT(0x52, (vaddr)&isr_wrapper_0x52);
  setupIDT(0x53, (vaddr)&isr_wrapper_0x53);
  setupIDT(0x54, (vaddr)&isr_wrapper_0x54);
  setupIDT(0x55, (vaddr)&isr_wrapper_0x55);
  setupIDT(0x56, (vaddr)&isr_wrapper_0x56);
  setupIDT(0x57, (vaddr)&isr_wrapper_0x57);
  setupIDT(0x58, (vaddr)&isr_wrapper_0x58);
  setupIDT(0x59, (vaddr)&isr_wrapper_0x59);
  setupIDT(0x5a, (vaddr)&isr_wrapper_0x5a);
  setupIDT(0x5b, (vaddr)&isr_wrapper_0x5b);
  setupIDT(0x5c, (vaddr)&isr_wrapper_0x5c);
  setupIDT(0x5d, (vaddr)&isr_wrapper_0x5d);
  setupIDT(0x5e, (vaddr)&isr_wrapper_0x5e);
  setupIDT(0x5f, (vaddr)&isr_wrapper_0x5f);
  setupIDT(0x60, (vaddr)&isr_wrapper_0x60);
  setupIDT(0x61, (vaddr)&isr_wrapper_0x61);
  setupIDT(0x62, (vaddr)&isr_wrapper_0x62);
  setupIDT(0x63, (vaddr)&isr_wrapper_0x63);
  setupIDT(0x64, (vaddr)&isr_wrapper_0x64);
  setupIDT(0x65, (vaddr)&isr_wrapper_0x65);
  setupIDT(0x66, (vaddr)&isr_wrapper_0x66);
  setupIDT(0x67, (vaddr)&isr_wrapper_0x67);
  setupIDT(0x68, (vaddr)&isr_wrapper_0x68);
  setupIDT(0x69, (vaddr)&isr_wrapper_0x69);
  setupIDT(0x6a, (vaddr)&isr_wrapper_0x6a);
  setupIDT(0x6b, (vaddr)&isr_wrapper_0x6b);
  setupIDT(0x6c, (vaddr)&isr_wrapper_0x6c);
  setupIDT(0x6d, (vaddr)&isr_wrapper_0x6d);
  setupIDT(0x6e, (vaddr)&isr_wrapper_0x6e);
  setupIDT(0x6f, (vaddr)&isr_wrapper_0x6f);
  setupIDT(0x70, (vaddr)&isr_wrapper_0x70);
  setupIDT(0x71, (vaddr)&isr_wrapper_0x71);
  setupIDT(0x72, (vaddr)&isr_wrapper_0x72);
  setupIDT(0x73, (vaddr)&isr_wrapper_0x73);
  setupIDT(0x74, (vaddr)&isr_wrapper_0x74);
  setupIDT(0x75, (vaddr)&isr_wrapper_0x75);
  setupIDT(0x76, (vaddr)&isr_wrapper_0x76);
  setupIDT(0x77, (vaddr)&isr_wrapper_0x77);
  setupIDT(0x78, (vaddr)&isr_wrapper_0x78);
  setupIDT(0x79, (vaddr)&isr_wrapper_0x79);
  setupIDT(0x7a, (vaddr)&isr_wrapper_0x7a);
  setupIDT(0x7b, (vaddr)&isr_wrapper_0x7b);
  setupIDT(0x7c, (vaddr)&isr_wrapper_0x7c);
  setupIDT(0x7d, (vaddr)&isr_wrapper_0x7d);
  setupIDT(0x7e, (vaddr)&isr_wrapper_0x7e);
  setupIDT(0x7f, (vaddr)&isr_wrapper_0x7f);
  setupIDT(0x80, (vaddr)&isr_wrapper_0x80);
  setupIDT(0x81, (vaddr)&isr_wrapper_0x81);
  setupIDT(0x82, (vaddr)&isr_wrapper_0x82);
  setupIDT(0x83, (vaddr)&isr_wrapper_0x83);
  setupIDT(0x84, (vaddr)&isr_wrapper_0x84);
  setupIDT(0x85, (vaddr)&isr_wrapper_0x85);
  setupIDT(0x86, (vaddr)&isr_wrapper_0x86);
  setupIDT(0x87, (vaddr)&isr_wrapper_0x87);
  setupIDT(0x88, (vaddr)&isr_wrapper_0x88);
  setupIDT(0x89, (vaddr)&isr_wrapper_0x89);
  setupIDT(0x8a, (vaddr)&isr_wrapper_0x8a);
  setupIDT(0x8b, (vaddr)&isr_wrapper_0x8b);
  setupIDT(0x8c, (vaddr)&isr_wrapper_0x8c);
  setupIDT(0x8d, (vaddr)&isr_wrapper_0x8d);
  setupIDT(0x8e, (vaddr)&isr_wrapper_0x8e);
  setupIDT(0x8f, (vaddr)&isr_wrapper_0x8f);
  setupIDT(0x90, (vaddr)&isr_wrapper_0x90);
  setupIDT(0x91, (vaddr)&isr_wrapper_0x91);
  setupIDT(0x92, (vaddr)&isr_wrapper_0x92);
  setupIDT(0x93, (vaddr)&isr_wrapper_0x93);
  setupIDT(0x94, (vaddr)&isr_wrapper_0x94);
  setupIDT(0x95, (vaddr)&isr_wrapper_0x95);
  setupIDT(0x96, (vaddr)&isr_wrapper_0x96);
  setupIDT(0x97, (vaddr)&isr_wrapper_0x97);
  setupIDT(0x98, (vaddr)&isr_wrapper_0x98);
  setupIDT(0x99, (vaddr)&isr_wrapper_0x99);
  setupIDT(0x9a, (vaddr)&isr_wrapper_0x9a);
  setupIDT(0x9b, (vaddr)&isr_wrapper_0x9b);
  setupIDT(0x9c, (vaddr)&isr_wrapper_0x9c);
  setupIDT(0x9d, (vaddr)&isr_wrapper_0x9d);
  setupIDT(0x9e, (vaddr)&isr_wrapper_0x9e);
  setupIDT(0x9f, (vaddr)&isr_wrapper_0x9f);
  setupIDT(0xa0, (vaddr)&isr_wrapper_0xa0);
  setupIDT(0xa1, (vaddr)&isr_wrapper_0xa1);
  setupIDT(0xa2, (vaddr)&isr_wrapper_0xa2);
  setupIDT(0xa3, (vaddr)&isr_wrapper_0xa3);
  setupIDT(0xa4, (vaddr)&isr_wrapper_0xa4);
  setupIDT(0xa5, (vaddr)&isr_wrapper_0xa5);
  setupIDT(0xa6, (vaddr)&isr_wrapper_0xa6);
  setupIDT(0xa7, (vaddr)&isr_wrapper_0xa7);
  setupIDT(0xa8, (vaddr)&isr_wrapper_0xa8);
  setupIDT(0xa9, (vaddr)&isr_wrapper_0xa9);
  setupIDT(0xaa, (vaddr)&isr_wrapper_0xaa);
  setupIDT(0xab, (vaddr)&isr_wrapper_0xab);
  setupIDT(0xac, (vaddr)&isr_wrapper_0xac);
  setupIDT(0xad, (vaddr)&isr_wrapper_0xad);
  setupIDT(0xae, (vaddr)&isr_wrapper_0xae);
  setupIDT(0xaf, (vaddr)&isr_wrapper_0xaf);
  setupIDT(0xb0, (vaddr)&isr_wrapper_0xb0);
  setupIDT(0xb1, (vaddr)&isr_wrapper_0xb1);
  setupIDT(0xb2, (vaddr)&isr_wrapper_0xb2);
  setupIDT(0xb3, (vaddr)&isr_wrapper_0xb3);
  setupIDT(0xb4, (vaddr)&isr_wrapper_0xb4);
  setupIDT(0xb5, (vaddr)&isr_wrapper_0xb5);
  setupIDT(0xb6, (vaddr)&isr_wrapper_0xb6);
  setupIDT(0xb7, (vaddr)&isr_wrapper_0xb7);
  setupIDT(0xb8, (vaddr)&isr_wrapper_0xb8);
  setupIDT(0xb9, (vaddr)&isr_wrapper_0xb9);
  setupIDT(0xba, (vaddr)&isr_wrapper_0xba);
  setupIDT(0xbb, (vaddr)&isr_wrapper_0xbb);
  setupIDT(0xbc, (vaddr)&isr_wrapper_0xbc);
  setupIDT(0xbd, (vaddr)&isr_wrapper_0xbd);
  setupIDT(0xbe, (vaddr)&isr_wrapper_0xbe);
  setupIDT(0xbf, (vaddr)&isr_wrapper_0xbf);
  setupIDT(0xc0, (vaddr)&isr_wrapper_0xc0);
  setupIDT(0xc1, (vaddr)&isr_wrapper_0xc1);
  setupIDT(0xc2, (vaddr)&isr_wrapper_0xc2);
  setupIDT(0xc3, (vaddr)&isr_wrapper_0xc3);
  setupIDT(0xc4, (vaddr)&isr_wrapper_0xc4);
  setupIDT(0xc5, (vaddr)&isr_wrapper_0xc5);
  setupIDT(0xc6, (vaddr)&isr_wrapper_0xc6);
  setupIDT(0xc7, (vaddr)&isr_wrapper_0xc7);
  setupIDT(0xc8, (vaddr)&isr_wrapper_0xc8);
  setupIDT(0xc9, (vaddr)&isr_wrapper_0xc9);
  setupIDT(0xca, (vaddr)&isr_wrapper_0xca);
  setupIDT(0xcb, (vaddr)&isr_wrapper_0xcb);
  setupIDT(0xcc, (vaddr)&isr_wrapper_0xcc);
  setupIDT(0xcd, (vaddr)&isr_wrapper_0xcd);
  setupIDT(0xce, (vaddr)&isr_wrapper_0xce);
  setupIDT(0xcf, (vaddr)&isr_wrapper_0xcf);
  setupIDT(0xd0, (vaddr)&isr_wrapper_0xd0);
  setupIDT(0xd1, (vaddr)&isr_wrapper_0xd1);
  setupIDT(0xd2, (vaddr)&isr_wrapper_0xd2);
  setupIDT(0xd3, (vaddr)&isr_wrapper_0xd3);
  setupIDT(0xd4, (vaddr)&isr_wrapper_0xd4);
  setupIDT(0xd5, (vaddr)&isr_wrapper_0xd5);
  setupIDT(0xd6, (vaddr)&isr_wrapper_0xd6);
  setupIDT(0xd7, (vaddr)&isr_wrapper_0xd7);
  setupIDT(0xd8, (vaddr)&isr_wrapper_0xd8);
  setupIDT(0xd9, (vaddr)&isr_wrapper_0xd9);
  setupIDT(0xda, (vaddr)&isr_wrapper_0xda);
  setupIDT(0xdb, (vaddr)&isr_wrapper_0xdb);
  setupIDT(0xdc, (vaddr)&isr_wrapper_0xdc);
  setupIDT(0xdd, (vaddr)&isr_wrapper_0xdd);
  setupIDT(0xde, (vaddr)&isr_wrapper_0xde);
  setupIDT(0xdf, (vaddr)&isr_wrapper_0xdf);
  setupIDT(0xe0, (vaddr)&isr_wrapper_0xe0);
  setupIDT(0xe1, (vaddr)&isr_wrapper_0xe1);
  setupIDT(0xe2, (vaddr)&isr_wrapper_0xe2);
  setupIDT(0xe3, (vaddr)&isr_wrapper_0xe3);
  setupIDT(0xe4, (vaddr)&isr_wrapper_0xe4);
  setupIDT(0xe5, (vaddr)&isr_wrapper_0xe5);
  setupIDT(0xe6, (vaddr)&isr_wrapper_0xe6);
  setupIDT(0xe7, (vaddr)&isr_wrapper_0xe7);
  setupIDT(0xe8, (vaddr)&isr_wrapper_0xe8);
  setupIDT(0xe9, (vaddr)&isr_wrapper_0xe9);
  setupIDT(0xea, (vaddr)&isr_wrapper_0xea);
  setupIDT(0xeb, (vaddr)&isr_wrapper_0xeb);
  setupIDT(0xec, (vaddr)&isr_wrapper_0xec);
  setupIDT(0xed, (vaddr)&isr_wrapper_0xed);
  setupIDT(0xee, (vaddr)&isr_wrapper_0xee);
  setupIDT(0xef, (vaddr)&isr_wrapper_0xef);
  setupIDT(0xf0, (vaddr)&isr_wrapper_0xf0);
  setupIDT(0xf1, (vaddr)&isr_wrapper_0xf1);
  setupIDT(0xf2, (vaddr)&isr_wrapper_0xf2);
  setupIDT(0xf3, (vaddr)&isr_wrapper_0xf3);
  setupIDT(0xf4, (vaddr)&isr_wrapper_0xf4);
  setupIDT(0xf5, (vaddr)&isr_wrapper_0xf5);
  setupIDT(0xf6, (vaddr)&isr_wrapper_0xf6);
  setupIDT(0xf7, (vaddr)&isr_wrapper_0xf7);
  setupIDT(0xf8, (vaddr)&isr_wrapper_0xf8); // spurious interrupt
  setupIDT(0xf9, (vaddr)&isr_wrapper_0xf9);
  setupIDT(0xfa, (vaddr)&isr_wrapper_0xfa);
  setupIDT(0xfb, (vaddr)&isr_wrapper_0xfb);
  setupIDT(0xfc, (vaddr)&isr_wrapper_0xfc);
  setupIDT(0xfd, (vaddr)&isr_wrapper_0xfd);
  setupIDT(0xfe, (vaddr)&isr_wrapper_0xfe);
  setupIDT(0xff, (vaddr)&isr_wrapper_0xff); // unknown, sometimes fired in bochs
}
