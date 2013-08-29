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
#include "mach/Machine.h"
#include "mach/PageManager.h"
#include "mach/PCI.h"
#include "mach/asm_functions.h"
#include "mach/isr_wrapper.h"
#include "dev/Serial.h"
#include "dev/Keyboard.h"
#include "dev/PIT.h"
#include "dev/RTC.h"
#include "dev/Screen.h"
#include "gdb/Gdb.h"
#include "kern/FrameManager.h"
#include "kern/Kernel.h"
#include "kern/Multiboot.h"
#include "kern/Thread.h"

// drivers
#include "mach/IOPortManager.h"
#include "mach/IRQManager.h"
#include "kern/Drivers.h"

#include <atomic>
#include <list>

// check various assumptions about data type sizes
static_assert(sizeof(uint64_t) == sizeof(mword), "mword != uint64_t" );
static_assert(sizeof(size_t) == sizeof(mword), "mword != size_t");
static_assert(sizeof(ptr_t) == sizeof(mword), "mword != ptr_t");
static_assert(sizeof(LAPIC) == 0x400, "sizeof(LAPIC) != 0x400" );
static_assert(sizeof(LAPIC) <= pagesize<1>(), "sizeof(LAPIC) <= pagesize<1>()" );
static_assert(sizeof(InterruptDescriptor) == 2 * sizeof(mword), "sizeof(InterruptDescriptor) != 2 * sizeof(mword)" );
static_assert(sizeof(SegmentDescriptor) == sizeof(mword), "sizeof(SegmentDescriptor) != sizeof(mword)" );

// symbols from boot.asm that point to 16-bit boot code location
extern const char boot16Begin, boot16End;

// symbols set during linking, see linker.ld
extern const char __Boot, __BootEnd;
extern const char __ctors_start, __ctors_end;
extern const char __KernelCode, __KernelCodeEnd;
extern const char __KernelRO,   __KernelRO_End;
extern const char __KernelData, __KernelDataEnd;
extern const char __KernelBss,  __KernelBssEnd;
extern const char __BootHeap,   __BootHeapEnd;

// screen memory and pointer to it (from Screen.h)
char Screen::buffer[xmax * ymax];
char* Screen::video = nullptr;

// interrupt descriptor tables
static const unsigned int maxIDT = 256;
InterruptDescriptor idt[maxIDT] __aligned(0x1000);

// static global objects
static FrameManager frameManager;
static Multiboot multiboot                        __section(".boot.data");
// flag to check for initial test IPI
static bool tipiReceived                          __section(".boot.data");

// processor/core information
Processor* Machine::processorTable = nullptr;
uint32_t Machine::cpuCount = 0;
uint32_t Machine::bspIndex = ~0;
uint32_t Machine::bspApicID = ~0;

// used to enumerate APs during bootstrap
static atomic<mword> apIndex;

// IRQ information
uint32_t Machine::irqCount = 0;
Machine::IrqInfo* Machine::irqTable = nullptr;
Machine::IrqOverrideInfo* Machine::irqOverrideTable = nullptr;

// static keyboard and RTC object
static Keyboard keyboard;
static PIT pit;
static RTC rtc;

extern "C" void cdi_init(ptr_t);

// simple thread to print keycode on screen
static void keybLoop(ptr_t) {
  for (;;) {
    Keyboard::KeyCode keycode = (keyboard.read());
#if KEYBOARD_RAW
    StdErr.out(' ', FmtHex(keycode));
#else
    if (keycode > 0 && keycode < 0x80) StdErr.out((char)keycode);
#endif
    Drivers::parseCommands(keyboard, (char)keycode);
  }
}

// init routine for APs, on boot stack and identity paging
void Machine::initAP(funcvoid_t func) {
  // init and install processor object
  processorTable[apIndex].init(frameManager, kernelSpace, idt, sizeof(idt));
  // start main/idle loop
  Processor::start(func);
}

// on proper stack, processor initialized
void Machine::initAP2() {
  Processor::enableInterrupts();         // enable interrupts (off boot stack)
  DBG::out(DBG::Basic, ' ', apIndex);
  apIndex = bspIndex;                    // sync with BSP, then halt
  Halt();
}

// init routine for BSP, on boot stack and identity paging
void Machine::initBSP(mword magic, vaddr mbiAddr, funcvoid_t func) {
  // initialize bss
  memset((char*)&__KernelBss, 0, &__KernelBssEnd - &__KernelBss);

  // initialize MBI & debugging: no debug output before this point!
  vaddr mbiEnd = multiboot.init(magic, mbiAddr + kernelBase);
  // determine end addresses of kernel overall (except modules)
  vaddr kernelEnd = align_up(mbiEnd, pagesize<2>());

  // initialize basic devices
  Screen::init(kernelBase);                        // no identity mapping later
  SerialDevice0::init(DBG::test(DBG::GDBEnable));  // no interrupts
  DebugDevice::init();                             // qemu debug device

  // give kernel heap pre-allocated memory -> limited dynamic memory available
  kernelHeap.init(vaddr(&__BootHeap), vaddr(&__BootHeapEnd));

  // need dummy processor for lock counter: no locking before this point!
  Processor dummyProcessor;
  dummyProcessor.initDummy(frameManager);

  // call global constructors: %rbx is callee-saved, thus safe to use
  // ostream output cannot be used before this point (i.e. not DBG::out)
  for ( const char* x = &__ctors_start; x != &__ctors_end; x += sizeof(char*)) {
    asm volatile( "call *(%0)" : : "b"(x) : "memory" );
  }

  // re-initialize debugging, but this time with messages
  DBG::outln(DBG::Basic, "********** BOOT **********");
  multiboot.initDebug();

  // check CPU capabilities
  DBG::out(DBG::Basic, "checking BSP capabilities:");
  Processor::checkCapabilities(true);

  // print boot memory information
  DBG::outln(DBG::Basic, "Boot16:    ", FmtHex(&boot16Begin),  " - ", FmtHex(&boot16End));
  DBG::outln(DBG::Basic, "PageTbls:  ", FmtHex(PageManager::ptp<1>()), " - ", FmtHex(PageManager::ptpend()));
  DBG::outln(DBG::Basic, "Kernel:    ", FmtHex(kernelBase),    " - ", FmtHex(kernelEnd));
  DBG::outln(DBG::Basic, "Boot Seg:  ", FmtHex(&__Boot),       " - ", FmtHex(&__BootEnd));
  DBG::outln(DBG::Basic, "Code Seg:  ", FmtHex(&__KernelCode), " - ", FmtHex(&__KernelCodeEnd));
  DBG::outln(DBG::Basic, "RO Seg:    ", FmtHex(&__KernelRO),   " - ", FmtHex(&__KernelRO_End));
  DBG::outln(DBG::Basic, "Data Seg:  ", FmtHex(&__KernelData), " - ", FmtHex(&__KernelDataEnd));
  DBG::outln(DBG::Basic, "Bss Seg:   ", FmtHex(&__KernelBss),  " - ", FmtHex(&__KernelBssEnd));
  DBG::outln(DBG::Basic, "Heap Seg:  ", FmtHex(&__BootHeap),   " - ", FmtHex(&__BootHeapEnd));
  DBG::outln(DBG::Basic, "Multiboot: ", FmtHex((mbiAddr + kernelBase)), " - ", FmtHex(mbiEnd));
  DBG::outln(DBG::Basic, "Vid/APIC:  ", FmtHex(videoAddr), " / ", FmtHex(lapicAddr));

  // get memory information from multiboot -> store in frameManager
  laddr modStart, modEnd;
  multiboot.parseAll(modStart, modEnd);
  multiboot.getMemory(frameManager);                 // get free memory
  DBG::outln(DBG::Basic, "FM/init: ", frameManager);
  multiboot.getModules(frameManager, pagesize<2>()); // reserve module memory
  DBG::outln(DBG::Basic, "FM/modules: ", frameManager);
  vaddr rsdp = multiboot.getRSDP() - kernelBase;     // get RSDP

  // marked boot memory as used
  bool check = frameManager.reserve(vaddr(&__Boot) - kernelBase, kernelEnd - vaddr(&__Boot));
  KASSERTN( check, FmtHex(vaddr(&__Boot) - kernelBase), '-', FmtHex(kernelEnd - vaddr(&__Boot)) );
  DBG::outln(DBG::Basic, "FM/alloc: ", frameManager);

  // copy boot code for APs
  check = frameManager.reserve( BOOT16, pagesize<1>() );
  KASSERTN( check, FmtHex(BOOT16), '-', FmtHex(pagesize<1>()) );
  memcpy((char*)BOOT16, &boot16Begin, vaddr(&boot16End) - vaddr(&boot16Begin));
  DBG::outln(DBG::Basic, "FM/boot16: ", frameManager);

  // bootstrap PageManager: permissive for boot segment (data, eg. rsp in boot)
  laddr pml4addr = PageManager::bootstrap( frameManager, kernelBase,
    vaddr(&__Boot), vaddr(&__KernelCode), vaddr(&__KernelData), kernelEnd,
    pagesize<2>() );
  DBG::outln(DBG::Basic, "FM/bootstrap: ", frameManager);

  // release all mapped memory (excl. multiboot) to heap
  kernelHeap.addMemory(align_up(mbiEnd, pagesize<1>()), kernelEnd);
  DBG::outln(DBG::Basic, "VM/bootstrap: ", kernelHeap);

  // init kernel address space
  kernelSpace.setPagetable(pml4addr);
  kernelSpace.setMemoryRange(kernelEnd, topkernel - kernelEnd);
  kernelSpace.activate();
  kernelHeap.init2();
  DBG::outln(DBG::Basic, "AS/bootstrap: ", kernelSpace);

  // initialize kernel file system with boot modules
  modStart = align_down(modStart, pagesize<2>());
  modEnd = align_up(modEnd, pagesize<2>());
  vaddr modMapped = kernelSpace.mapPages<2>(modStart, modEnd - modStart, AddressSpace::Data);
  KASSERTN(modMapped != topaddr, FmtHex(modStart), '-', FmtHex(modEnd));
  multiboot.readModules(modMapped - modStart, frameManager, pagesize<2>());
  check = kernelSpace.unmapPages<2>(modMapped, modEnd - modStart);
  KASSERTN(check, FmtHex(modMapped), '-', FmtHex(modEnd - modStart));
  DBG::outln(DBG::Basic, "FM/modules: ", frameManager);
  DBG::outln(DBG::Basic, "AS/modules: ", kernelSpace);

  // release multiboot memory to heap
  kernelHeap.addMemory(vaddr(&__BootHeapEnd), align_up(mbiEnd, pagesize<1>()));
  DBG::outln(DBG::Basic, "VM/mbi: ", kernelHeap);

  // install global IDT entries -> need to be done, before processor init
  setupAllIDTs();

  // parse ACPI tables: find/initialize CPUs, APICs, IOAPICs, static devices
  laddr apicPhysAddr = initACPI(rsdp);

  // initialize GDB object
  Gdb::init(cpuCount);

  // remap screen page, before boot memory disappears (later)
  PageManager::map<1>(videoAddr, PageManager::vtol(Screen::getAddress()), PageManager::Kernel, PageManager::Data, frameManager);
  Screen::setAddress(videoAddr);

  // map APIC page and enable local APIC, get bsp's apic ID
  PageManager::map<1>(lapicAddr, apicPhysAddr, PageManager::Kernel, PageManager::Data, frameManager);
  Processor::enableAPIC();
  bspApicID = Processor::getLAPIC_ID();
  for (size_t i = 0; i < cpuCount; i += 1) {
    if (processorTable[i].rApicID() == bspApicID) bspIndex = i;
  }
  DBG::outln(DBG::Basic, "CPUs: ", cpuCount, '/', bspIndex, '/', bspApicID);

  // init and install processor object
  processorTable[bspIndex].init(frameManager, kernelSpace, idt, sizeof(idt));

  // start gdb -> need to have IDT installed
  Gdb::start();

  // set up RTC & PIT
  rtc.init();
  pit.init();

  // init IOPortManager
  IOPortManager::init(0, 0x3ffff); // hope 256KB is enough

  // detect PCI controller
  PCI::sanityCheck();

  // find PCI devices
  PCI::probeAll();
  PCI::checkAllBuses();

  // find additional devices ("current thread" faked for ACPI)
  parseACPI();

  // start main/idle loop
  Processor::start(func);
}

// on proper stack, processor initialized
void Machine::initBSP2() {
  // enable interrupts (off boot stack)
  Processor::enableInterrupts();

  // initialize CDI (run on separate thread because Thread::sleep() won't work as expected if it runs on the idle thread
  Thread* cdiThread = Thread::create(kernelSpace, "CDI thread");
  unsigned int* numIRQ = new unsigned int(IRQManager::getNumIRQ());
  kernelScheduler.run(*cdiThread, cdi_init, numIRQ);

  // with interrupts enabled (needed later for timeouts): set up keyboard
  keyboard.init();

  // send test IPI to self
  tipiReceived = false;
  processorTable[bspIndex].sendTestIPI();
  rtc.wait(5);
  KASSERT0(tipiReceived);

  // start up APs (must be off boot stack): APs go into long mode and halt
  DBG::out(DBG::Basic, "AP init:");
  for ( uint32_t i = 0; i < cpuCount; i += 1 ) {
    if ( i != bspIndex ) {
      apIndex = i;                          // prepare sync variable
      processorTable[i].sendInitIPI();
      rtc.wait(5);                          // wait for HW init
      processorTable[i].sendStartupIPI(BOOT16 / 0x1000);
      while (apIndex != bspIndex) Pause();  // wait for SW init - see initAP()
    }
  }
  DBG::out(DBG::Basic, kendl);

  // free kernel boot memory
  for ( vaddr x = kernelBase; x < vaddr(&__KernelCode); x += pagesize<2>() ) {
    PageManager::unmap<2>(x, frameManager);
  }
  bool check = frameManager.release(vaddr(&__Boot) - kernelBase, vaddr(&__KernelCode) - vaddr(&__Boot));
  KASSERTN( check, FmtHex(vaddr(&__Boot) - kernelBase), '-', FmtHex(vaddr(&__KernelCode) - vaddr(&__Boot)) );

  // free AP boot code
  check = frameManager.release(BOOT16, pagesize<1>());
  KASSERTN( check, FmtHex(BOOT16), '-', FmtHex(pagesize<1>()) );
  DBG::outln(DBG::Basic, "FM/free16: ", frameManager);
  DBG::outln(DBG::Basic, "AS/free16: ", kernelSpace);
  DBG::outln(DBG::Basic, "VM/free16: ", kernelHeap);

  // wake up APs
  for ( uint32_t i = 0; i < cpuCount; i += 1 ) {
    if ( i != bspIndex ) processorTable[i].sendWakeUpIPI();
  }

  // create simple keyboard-to-screen thread
  Thread* t = Thread::create(kernelSpace, "keyb")->setPriority(1);
  kernelScheduler.run(*t, keybLoop, nullptr);
}

void Machine::staticEnableIRQ( mword irqnum, mword idtnum, bool low, bool level ) {
  KASSERT1(irqnum < PIC::Max, irqnum);
  irqnum = irqOverrideTable[irqnum].global;
  // TODO: handle irq override flags
  volatile IOAPIC* ioapic = (IOAPIC*)kernelSpace.mapPages<1>( irqTable[irqnum].ioApicAddr, pagesize<1>(), AddressSpace::Data );
  DBG::outln(DBG::Basic, "IRQ: ", FmtHex(irqTable[irqnum].ioapicIrq), " -> ", FmtHex(idtnum) );
  ioapic->mapIRQ( irqTable[irqnum].ioapicIrq, idtnum, bspApicID, low, level );
  kernelSpace.unmapPages<1>( (vaddr)ioapic, pagesize<1>() );
}

mword Machine::now() {
  return pit.tick();
}

inline void Machine::rtcInterrupt(mword counter) {
  if (counter % cpuCount == bspIndex) {
    if (Processor::preempt()) kernelScheduler.preempt();
  } else {
    processorTable[counter % cpuCount].sendWakeUpIPI();
  }
}

extern "C" void syscall_handler(mword x) {
	DBG::outln(DBG::Process, "system call: ", x);
	if (x == 3) kernelScheduler.kill();
}

extern "C" void isr_handler_0x08() { // double fault
  StdErr.outln("\nDOUBLE FAULT");
  Reboot( 1 );
}

extern "C" void isr_handler_0x0c(mword errcode, vaddr iAddr) { // stack fault
  StdErr.out("\nSTACK FAULT - ia: ", FmtHex(iAddr), " / sp: ", FmtHex(CPU::readRSP()), " / flags: ", FmtHex(errcode));
  Reboot( iAddr );
}

extern "C" void isr_handler_0x0d(mword errcode, vaddr iAddr) { // GP fault
  StdErr.outln("\nGENERAL PROTECTION FAULT - ia: ", FmtHex(iAddr), " / flags: ", FmtHex(errcode));
  Reboot( iAddr );
}

extern "C" void isr_handler_0x0e(mword errcode, vaddr iAddr) { // page fault
  StdErr.outln("\nPAGE FAULT - ia: ", FmtHex(iAddr), " / da: ", FmtHex(CPU::readCR2()), " / flags: ", FmtHex(errcode));
  Reboot( iAddr );
}

extern "C" void isr_handler_0x20() { // PIT interrupt
  pit.staticInterruptHandler();      // low-level processing before EOI
  Processor::sendEOI();
  if (Processor::preempt()) kernelScheduler.timerEvent(pit.tick());
}

extern "C" void isr_handler_0x21() { // keyboard interrupt
  keyboard.staticInterruptHandler(); // low-level processing before EOI
  Processor::sendEOI();
}

extern "C" void isr_handler_0x27() { // parallel interrupt, spurious no problem
  StdErr.out(" parallel");
  Processor::sendEOI();
}

extern "C" void isr_handler_0x28() { // RTC interrupt
  rtc.staticInterruptHandler();      // low-level processing before EOI
  Processor::sendEOI();              // EOI *before* switching stacks
  if (rtc.tick() % 16 == 0) Machine::rtcInterrupt(rtc.tick() / 16);
}

extern "C" void isr_handler_0x2c() { // mouse interrupt
  StdErr.out(" mouse");
  Processor::sendEOI();
}

extern "C" void isr_handler_0x40() {
  Processor::sendEOI();              // EOI *before* switching stacks
  KASSERT0(!Processor::interruptsEnabled());
  if (Processor::preempt()) kernelScheduler.preempt();
}

extern "C" void isr_handler_0x41() {
  KASSERT0(!Processor::interruptsEnabled());
  StdErr.out(" TIPI");
  tipiReceived = true;
  Processor::sendEOI();
}

extern "C" void isr_handler_0xf8() {
  StdErr.out(" spurious");
  Processor::sendEOI();
}

extern "C" void isr_handler_0xff() {
  StdErr.out(" 0xFF");
  Processor::sendEOI();
}

extern "C" void isr_handler_gen(mword num) {
  bool handled = IRQManager::handleIRQ(num);
  if (!handled) {
    StdErr.outln("\nUNEXPECTED INTERRUPT: ", FmtHex(num), '/', FmtHex(CPU::readCR2()));
  }
  Processor::sendEOI();
//  Reboot();
}

extern "C" void isr_handler_gen_err(mword num, mword errcode) {
  StdErr.outln("\nUNEXPECTED INTERRUPT: ", FmtHex(num), '/', FmtHex(CPU::readCR2()), '/', FmtHex(errcode));
  Processor::sendEOI();
//  Reboot();
}

void Machine::setupIDT(unsigned int number, laddr address) {
  KASSERT1(number < maxIDT, number);
  idt[number].Offset00 = (address & 0x000000000000FFFF);
  idt[number].Offset16 = (address & 0x00000000FFFF0000) >> 16;
  idt[number].Offset32 = (address & 0xFFFFFFFF00000000) >> 32;
  idt[number].SegmentSelector = Processor::kernCodeSelector * sizeof(SegmentDescriptor);
  idt[number].Type = 0x0E; // 64-bit interrupt gate (trap does not disable interrupts)
  idt[number].P = 1;
}

void Machine::setupAllIDTs() {
  KASSERT0(!Processor::interruptsEnabled());
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
