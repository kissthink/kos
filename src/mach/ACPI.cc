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
extern "C" {
#include "extern/acpica/source/include/acpi.h"
}
#include "util/Output.h"
#include "mach/Machine.h"
#include "mach/Processor.h"
#include "mach/PCI.h"
#include "kern/AddressSpace.h"
#include "kern/Kernel.h"
#include "ipc/BlockingSync.h"

#include <map>

#undef __STRICT_ANSI__ // to get access to vsnprintf
#include <cstdio>

static vaddr rsdp = 0;

static map<vaddr,mword,less<vaddr>,KernelAllocator<pair<vaddr,mword>>> allocations;
static map<vaddr,size_t,less<vaddr>,KernelAllocator<pair<vaddr,size_t>>> mappings;

static ACPI_OSD_HANDLER sciHandler = nullptr;
static void* sciContext = nullptr;

extern "C" void isr_handler_0x29() { // SCI interrupt
  if (sciHandler) {
    UINT32 retCode = sciHandler(sciContext);
    KASSERT1(retCode == ACPI_INTERRUPT_HANDLED, retCode);
  }
  Processor::sendEOI();
}

laddr Machine::initACPI(vaddr r) {
  // set up information for acpica callback
  rsdp = r;

  // initialize acpi tables
  ACPI_STATUS status = AcpiInitializeTables( NULL, 0, true );
  KASSERT1( status == AE_OK, status );

  // FADT is needed to check for PS/2 keyboard
  acpi_table_fadt* fadt;
  status = AcpiGetTable( const_cast<char*>(ACPI_SIG_FADT), 0, (ACPI_TABLE_HEADER**)&fadt );
  KASSERT1( status == AE_OK, status );
  DBG::out(DBG::Acpi, "FADT: ", fadt->Header.Length, '/', FmtHex(fadt->BootFlags));
  if (fadt->BootFlags & ACPI_FADT_8042) {
    DBG::out(DBG::Acpi, " - 8042");
    // enable PS/2 driver, but: FADT bit not set on bochs & qemu
  }
  DBG::out(DBG::Acpi, kendl);

  acpi_table_srat* srat;
  status = AcpiGetTable( const_cast<char*>(ACPI_SIG_SRAT), 0, (ACPI_TABLE_HEADER**)&srat );
  if (status == AE_OK) {
    DBG::outln(DBG::Acpi, "SRAT: ", srat->Header.Length);
  }

  acpi_table_slit* slit;
  status = AcpiGetTable( const_cast<char*>(ACPI_SIG_SLIT), 0, (ACPI_TABLE_HEADER**)&slit );
  if (status == AE_OK) {
    DBG::outln(DBG::Acpi, "SLIT: ", slit->Header.Length);
  }

  // MADT is needed for APICs, I/O-APICS, etc.
  acpi_table_madt* madt;
  status = AcpiGetTable( const_cast<char*>(ACPI_SIG_MADT), 0, (ACPI_TABLE_HEADER**)&madt );
  KASSERT1( status == AE_OK, status );
  int madtLength = madt->Header.Length - sizeof(acpi_table_madt);
  laddr apicPhysAddr = madt->Address;
  DBG::out(DBG::Acpi, "MADT: ", madtLength, '/', FmtHex(apicPhysAddr));

  // disable 8259 PIC, if present
  if (madt->Flags & ACPI_MADT_PCAT_COMPAT) {
    DBG::out(DBG::Acpi, " - 8259");
    PIC::disable();
  }

  map<uint32_t,uint32_t,less<uint32_t>,KernelAllocator<pair<uint32_t,uint32_t>>> apicMap;
  map<uint32_t,IrqInfo,less<uint32_t>,KernelAllocator<pair<uint32_t,IrqInfo>>> irqMap;
  map<uint32_t,IrqOverrideInfo,less<uint32_t>,KernelAllocator<pair<uint32_t,IrqOverrideInfo>>> irqOverrideMap;

  // walk through subtables and gather information in dynamic maps
  acpi_subtable_header* subtable = (acpi_subtable_header*)(madt + 1);
  while (madtLength > 0) {
    KASSERTN(subtable->Length <= madtLength, subtable->Length, '/', madtLength);
    switch (subtable->Type) {
    case ACPI_MADT_TYPE_LOCAL_APIC: {
      acpi_madt_local_apic* la = (acpi_madt_local_apic*)subtable;
      if (la->LapicFlags & 0x1) apicMap.insert( {la->ProcessorId, la->Id} );
    } break;
    case ACPI_MADT_TYPE_IO_APIC: {
      acpi_madt_io_apic* io = (acpi_madt_io_apic*)subtable;
      // map IO APIC into virtual address space
      volatile IOAPIC* ioapic = (IOAPIC*)kernelSpace.mapPages<1>(laddr(io->Address), pagesize<1>(), AddressSpace::Data );
      // get number of redirect entries, adjust irqCount
      uint8_t rdr = ioapic->getRedirects() + 1;
      if ( io->GlobalIrqBase + rdr > irqCount ) irqCount = io->GlobalIrqBase + rdr;
      // mask all interrupts and store irq/ioapic information
      for (uint8_t x = 0; x < rdr; x += 1 ) {
        ioapic->maskIRQ(x);
        irqMap.insert( {io->GlobalIrqBase + x, {io->Address, x}} );
      }
      // unmap IO APIC
      kernelSpace.unmapPages<1>( (vaddr)ioapic, pagesize<1>() );
    } break;
    case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE: {
      acpi_madt_interrupt_override* io = (acpi_madt_interrupt_override*)subtable;
      irqOverrideMap.insert( {io->SourceIrq, {io->GlobalIrq, io->IntiFlags}} );
    } break;
    case ACPI_MADT_TYPE_NMI_SOURCE:
      DBG::out(DBG::Acpi, " NMI_SOURCE"); break;
    case ACPI_MADT_TYPE_LOCAL_APIC_NMI: {
      DBG::out(DBG::Acpi, " LOCAL_APIC_NMI");
      acpi_madt_local_apic_nmi* ln = (acpi_madt_local_apic_nmi*)subtable;
      DBG::out(DBG::Acpi, '/', int(ln->ProcessorId), '/', int(ln->IntiFlags), '/', int(ln->Lint) );
    } break;
    case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE: {
      acpi_madt_local_apic_override* ao = (acpi_madt_local_apic_override*)subtable;
      apicPhysAddr = ao->Address;
      } break;
    case ACPI_MADT_TYPE_IO_SAPIC:
      DBG::out(DBG::Acpi, " IO_SAPIC"); break;
    case ACPI_MADT_TYPE_LOCAL_SAPIC:
      DBG::out(DBG::Acpi, " LOCAL_SAPIC"); break;
    case ACPI_MADT_TYPE_INTERRUPT_SOURCE:
      DBG::out(DBG::Acpi, " INTERRUPT_SOURCE"); break;
    case ACPI_MADT_TYPE_LOCAL_X2APIC:
      DBG::out(DBG::Acpi, " LOCAL_X2APIC"); break;
    case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI:
      DBG::out(DBG::Acpi, " LOCAL_X2APIC_NMI"); break;
    case ACPI_MADT_TYPE_GENERIC_INTERRUPT:
      DBG::out(DBG::Acpi, " GENERIC_INTERRUPT"); break;
    case ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR:
      DBG::out(DBG::Acpi, " GENERIC_DISTRIBUTOR"); break;
    default: ABORT1("unknown ACPI MADT subtable");
    }
    madtLength -= subtable->Length;
    subtable = (acpi_subtable_header*)(((char*)subtable) + subtable->Length);
  }
  DBG::out(DBG::Acpi, kendl);

  // determine cpuCount and create processorTable
  cpuCount = apicMap.size();
  KASSERT0(cpuCount);
  processorTable = new Processor[cpuCount];
  int idx = 0;
  for (const pair<uint32_t,uint32_t>& ap : apicMap) {
    processorTable[idx].init(ap.second, ap.first);
    idx += 1;
  }

  // fill irqTable & irqOverrideTable
  KASSERT1(irqCount >= PIC::Max, irqCount );         // have at least PIC irqs
  irqTable = new IrqInfo[irqCount];
  irqOverrideTable = new IrqOverrideInfo[irqCount];
  for (uint32_t i = 0; i < irqCount; i += 1) {
    irqTable[i] = { 0, 0 };
    irqOverrideTable[i] = { i, 0 };
  }
  for (const pair<uint32_t,IrqInfo>& i : irqMap) {
    KASSERTN( i.first < irqCount, i.first, ' ', irqCount );
    irqTable[i.first] = i.second;
  }
  for (const pair<uint32_t,IrqOverrideInfo>& i : irqOverrideMap) {
    KASSERTN( i.first < irqCount, i.first, ' ', irqCount )
    irqOverrideTable[i.first] = i.second;
  }

  return apicPhysAddr;
}

static ACPI_STATUS display(ACPI_HANDLE handle, UINT32 level, void* ctx, void** retval) {
  ACPI_STATUS status;
  ACPI_DEVICE_INFO* info;
  ACPI_BUFFER path;
  char buffer[256];

  path.Length = sizeof(buffer);
  path.Pointer = buffer;
  status = AcpiGetName(handle, ACPI_FULL_PATHNAME, &path);
  KASSERT1( status == AE_OK, status );
  status = AcpiGetObjectInfo(handle, &info);
  KASSERT1( status == AE_OK, status );
//  AcpiOsPrintf("%.4s HID: %s, ADR: %.8X, Status: %x\n", (char*)&info->Name,
//    info->HardwareId.String, info->Address, info->CurrentStatus);
  return AE_OK;
}

void Machine::parseACPI() {
  ACPI_STATUS status;

  status = AcpiInitializeSubsystem();
  KASSERT1( status == AE_OK, status );

//  status = AcpiReallocateRootTable();
//  KASSERT1( status == AE_OK, status );  

  status = AcpiLoadTables();
  KASSERT1( status == AE_OK, status );

  // TODO: install notification handlers

  status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
  KASSERT1( status == AE_OK, status );

  status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
  KASSERT1( status == AE_OK, status );

  status = AcpiWalkNamespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX, display, NULL, NULL, NULL);
  KASSERT1( status == AE_OK, status );

  // TODO: in principle, the ACPI subsystem keeps running...
  status = AcpiTerminate();
  KASSERT1( status == AE_OK, status );

  // clean up ACPI leftover memory allocations
  for (pair<vaddr,mword> a : allocations ) {
    kernelHeap.free(ptr_t(a.first));
  }
  allocations.clear();
  for (pair<vaddr,size_t> m : mappings ) {
    kernelSpace.unmapPages<1>(m.first, m.second);
  }
  mappings.clear();
}

ACPI_STATUS AcpiOsInitialize(void) { return AE_OK; }

ACPI_STATUS AcpiOsTerminate(void) { return AE_OK; }

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
  return rsdp;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES* InitVal,
  ACPI_STRING* NewVal) {
  *NewVal = nullptr;
  return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER* ExistingTable,
  ACPI_TABLE_HEADER** NewTable) {
  *NewTable = nullptr;
  return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER* ExistingTable,
  ACPI_PHYSICAL_ADDRESS* NewAddress, UINT32* NewTableLength) {
  *NewAddress = 0;
  *NewTableLength = 0;
  return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* OutHandle) {
  *OutHandle = new SpinLock;
  return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) {
  delete reinterpret_cast<SpinLock*>(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
  reinterpret_cast<SpinLock*>(Handle)->acquire();
  return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
  reinterpret_cast<SpinLock*>(Handle)->release();
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits,
  ACPI_SEMAPHORE* OutHandle) {
  *OutHandle = new Semaphore(InitialUnits);
  return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
  delete reinterpret_cast<Semaphore*>(Handle);
  return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units,
  UINT16 Timeout) {

  KASSERT1(Timeout == 0xFFFF || Timeout == 0, Timeout);
  for (UINT32 x = 0; x < Units; x += 1) reinterpret_cast<Semaphore*>(Handle)->P();
  return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
  for (UINT32 x = 0; x < Units; x += 1) reinterpret_cast<Semaphore*>(Handle)->V();
  return AE_OK;
}

#if 0
ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX* OutHandle) {
  ABORT1(false,"");
  return AE_ERROR;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle) { ABORT1(false,""); }

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {
  ABORT1(false,"");
  return AE_ERROR;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle) { ABORT1(false,""); }
#endif

void* AcpiOsAllocate(ACPI_SIZE Size) {
  DBG::out(DBG::MemAcpi, "acpi alloc: ", Size);
  void* ptr = kernelHeap.malloc(Size);
  DBG::outln(DBG::MemAcpi, " -> ", ptr);
  memset(ptr, 0, Size); // zero out memory to play it safe...
  allocations.insert( {vaddr(ptr), Size} );
  return ptr;
}

void AcpiOsFree(void* Memory) {
  DBG::outln(DBG::MemAcpi, "acpi free: ", Memory);
  kernelHeap.free(Memory);
  allocations.erase( vaddr(Memory) );
}

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length) {
  DBG::out(DBG::MemAcpi, "acpi map: ", FmtHex(Where), '/', FmtHex(Length));
  laddr lma;
  size_t size;
  vaddr vma;
  if (Length == uint32_t(-1)) { // bochs quirk
    lma = align_down(laddr(Where), pagesize<2>());
    size = align_up(1 + laddr(Where) - lma, pagesize<2>());
    vma = kernelSpace.mapPages<2>(lma, size, AddressSpace::Data);
  } else {
    lma = align_down(laddr(Where), pagesize<1>());
    size = align_up(Length + laddr(Where) - lma, pagesize<1>());
    vma = kernelSpace.mapPages<1>(lma, size, AddressSpace::Data);
  }
  mappings.insert( pair<vaddr,size_t>(vma,size) );
  DBG::outln(DBG::MemAcpi, " -> ", FmtHex(vma));
  return (void*)(vma + Where - lma);
}

void AcpiOsUnmapMemory(void* LogicalAddress, ACPI_SIZE Size) {
  DBG::outln(DBG::MemAcpi, "acpi unmap: ", FmtHex(LogicalAddress), '/', FmtHex(Size));
  vaddr vma;
  size_t size;
  if (Size == uint32_t(-1)) { // bochs quirk
    vma = align_down(vaddr(LogicalAddress), pagesize<2>());
    size = align_up(1 + vaddr(LogicalAddress) - vma, pagesize<2>());
    kernelSpace.unmapPages<2>(vma, size);
  } else {
    vma = align_down(vaddr(LogicalAddress), pagesize<1>());
    size = align_up(Size + vaddr(LogicalAddress) - vma, pagesize<1>());
    kernelSpace.unmapPages<1>(vma, size);
  }
  mappings.erase(vma);
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void* LogicalAddress,
  ACPI_PHYSICAL_ADDRESS* PhysicalAddress) {
  *PhysicalAddress = PageManager::vtol(vaddr(LogicalAddress));
  return AE_OK;
}

// the cache mechanism is bypassed in favour of regular allocation
// the object size is encoded in fake cache pointer
ACPI_STATUS AcpiOsCreateCache(char* CacheName, UINT16 ObjectSize,
  UINT16 MaxDepth, ACPI_CACHE_T** ReturnCache) {
  *ReturnCache = (ACPI_CACHE_T*)(uintptr_t)ObjectSize;
  return AE_OK;
}

ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T* Cache) {
  return AE_OK;
}

ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T* Cache) {
  return AE_OK;
}

void* AcpiOsAcquireObject(ACPI_CACHE_T* Cache) {
  void* addr = (void*)kernelHeap.alloc((UINT16)(uintptr_t)Cache);
  memset((void*)addr, 0, (UINT16)(uintptr_t)Cache);
  return (void*)addr;
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T* Cache, void* Object) {
  kernelHeap.release((vaddr)Object, (UINT16)(uintptr_t)Cache);
  return AE_OK;
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber,
  ACPI_OSD_HANDLER ServiceRoutine, void* Context) {

  DBG::outln(DBG::Acpi, "ACPI install intr handler: ", InterruptNumber);
  if (InterruptNumber != 9 || !ServiceRoutine) return AE_BAD_PARAMETER;
  if (sciHandler) return AE_ALREADY_EXISTS;
  sciContext = Context;
  sciHandler = ServiceRoutine;
  return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber,
  ACPI_OSD_HANDLER ServiceRoutine) {

  DBG::outln(DBG::Acpi, "ACPI remove intr handler: ", InterruptNumber);
  if (!sciHandler) return AE_NOT_EXIST;
  if (InterruptNumber != 9 || !ServiceRoutine || ServiceRoutine != sciHandler) {
    return AE_BAD_PARAMETER;
  }
  sciHandler = nullptr;
  sciContext = nullptr;
  return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
  return (ACPI_THREAD_ID)Processor::getCurrThread();
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
  ACPI_OSD_EXEC_CALLBACK Function, void* Context) {

  ABORT0();
  return AE_ERROR;
}

void AcpiOsWaitEventsComplete(void* Context) { ABORT0(); }

void AcpiOsSleep(UINT64 Milliseconds) { return; ABORT0(); } // HP netbook

void AcpiOsStall(UINT32 Microseconds) { return; ABORT0(); } // AMD 2427

void AcpiOsWaitEventsComplete() { ABORT0(); }

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32* Value, UINT32 Width) {
  switch (Width) {
    case 8: *Value = in8(Address); break;
    case 16: *Value = in16(Address); break;
    case 32: *Value = in32(Address); break;
    default: return AE_ERROR;
  }
  return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
  switch (Width) {
    case 8: out8(Address,Value); break;
    case 16: out16(Address,Value); break;
    case 32: out32(Address,Value); break;
    default: return AE_ERROR;
  }
  return AE_OK;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64* Value, UINT32 Width) {
  ABORT0();
  return AE_ERROR;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
  ABORT0();
  return AE_ERROR;
}

ACPI_STATUS AcpiOsReadPciConfiguration( ACPI_PCI_ID* PciId, UINT32 Reg,
  UINT64* Value, UINT32 Width) {

  // PciId->Segment ?
  *Value = PCI::readConfigWord(PciId->Bus, PciId->Device, PciId->Function, Reg, Width);
  return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg,
  UINT64 Value, UINT32 Width) {

  // PciId->Segment ?
  PCI::writeConfigWord(PciId->Bus, PciId->Device, PciId->Function, Reg, Width, Value);
  return AE_OK;
}

BOOLEAN AcpiOsReadable(void* Pointer, ACPI_SIZE Length) {
  ABORT0();
  return false;
}

BOOLEAN AcpiOsWritable(void* Pointer, ACPI_SIZE Length) {
  ABORT0();
  return false;
}

UINT64 AcpiOsGetTimer(void) {
  ABORT0();
  return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void* Info) {
  ABORT0();
  return AE_ERROR;
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char* Format, ...) {
  va_list args;
  va_start(args, Format);
  AcpiOsVprintf(Format, args);
  va_end(args);
}

void AcpiOsVprintf(const char* Format, va_list Args) {
  va_list tmpargs;
  va_copy(tmpargs, Args);
  int size = vsnprintf(nullptr, 0, Format, tmpargs);
  va_end(tmpargs);
  if (size < 0) return;
  size += 1;
  char* buffer = new char[size];
  vsnprintf(buffer, size, Format, Args);
  DBG::out(DBG::Acpi, buffer);
  globaldelete(buffer, size);
}

void AcpiOsRedirectOutput(void* Destination) { ABORT0(); }

ACPI_STATUS AcpiOsGetLine(char* Buffer, UINT32 BufferLength, UINT32* BytesRead) {
  ABORT0();
  return AE_ERROR;
}

void* AcpiOsOpenDirectory(char* Pathname, char* WildcardSpec, char RequestedFileType) {
  ABORT0();
  return nullptr;
}

char* AcpiOsGetNextFilename(void* DirHandle) {
  ABORT0();
  return nullptr;
}

void AcpiOsCloseDirectory(void* DirHandle) { ABORT0(); }
