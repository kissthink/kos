#ifndef Gdb_h_
#define Gdb_h_ 1

#include "gdb/GdbCpu.h"
#include "gdb/NonBlockSemaphore.h"
#include "mach/CPU.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "kern/Debug.h"
#include "ipc/SpinLock.h"
#include "world/File.h"
#include "kern/Kernel.h"    // required by elfio.hpp
#include "extern/elfio/elfio.hpp"

extern void set_debug_traps(bool);
extern void breakpoint();

class Gdb {
public:
  static void init(int numCPU) {
    if (!DBG::test(DBG::GDBEnable)) return;
    KASSERT1(numCPU > 0, numCPU);
    numCpu = numCPU;
    gdbCPUs = new GdbCpu[numCpu];
    sem = new NonBlockSemaphore[numCpu];
    for (int i = 0; i < numCpu; i++) gdbCPUs[i].setCpuId(i);
    DBG::outln(DBG::GDBDebug, "Gdb state initialized for ", numCpu, " cores");
//    unWindDebugHookAddr = findUnwindDebugHookAddr();
//    KASSERT0(unWindDebugHookAddr);
//    DBG::outln(DBG::GDBDebug, "_Unwind_DebugHook address: ", FmtHex(unWindDebugHookAddr));
  }
  static void setUnwindDebugHookAddr(mword addr) {
    unWindDebugHookAddr = addr;
    DBG::outln(DBG::Basic, "_Unwind_DebugHook address: ", FmtHex(unWindDebugHookAddr));
  }

  // gives pointer to asked GdbCpu object
  static inline GdbCpu* getCurrentCpu() {
    return Processor::getGdbCpu();
  }
  static inline GdbCpu* getCpu(int cpuIdx) {
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return &gdbCPUs[cpuIdx];
  }
  static inline void setCpuState(cpuState::cpuStateEnum state) {
    Processor::getGdbCpu()->setCpuState(state);
  }

  // set current gdb CPU state
  static inline GdbCpu* setupGdb(int cpuIdx) {
    if (!DBG::test(DBG::GDBEnable)) return nullptr;
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    numInitialized = cpuIdx+1;
    gdbCPUs[cpuIdx].setCpuState(cpuState::RUNNING);
    return &gdbCPUs[cpuIdx];
  }

  // enumerate CPUs
  static inline void startEnumerate() {
    ScopedLock<> so(mutex);
    enumIdx = 0;
  }
  static inline GdbCpu* next() {
    ScopedLock<> so(mutex);
    if (enumIdx < numCpu) return &gdbCPUs[enumIdx++];
    return nullptr;
  }

  // returns buffer storing gdb manipulated registers for current CPU
  static inline char* getRegs64() {
    return reinterpret_cast<char *>(getCurrentCpu()->getRegs64());
  }
  static inline char* getRegs32() {
    return reinterpret_cast<char *>(getCurrentCpu()->getRegs32());
  }
  static inline char* getRegs64(int cpuIdx) {
    return reinterpret_cast<char *>(getCpu(cpuIdx)->getRegs64());
  }
  static inline char* getRegs32(int cpuIdx) {
    return reinterpret_cast<char *>(getCpu(cpuIdx)->getRegs32());
  }
  static inline reg64 getReg64(int regno) {
    return getCurrentCpu()->getRegs64()[regno];
  }
  static inline reg32 getReg32(int regno) {
    return getCurrentCpu()->getRegs32()[regno];
  }
  static inline reg64 getReg64(int regno, int cpuIdx) {
    return getCpu(cpuIdx)->getRegs64()[regno];
  }
  static inline reg32 getReg32(int regno, int cpuIdx) {
    return getCpu(cpuIdx)->getRegs32()[regno];
  }
  static inline void setReg64(int regno, reg64 val) {
    getCurrentCpu()->setReg64(regno, val);
  }
  static inline void setReg32(int regno, reg32 val) {
    getCurrentCpu()->setReg32(regno, val);
  }
  static inline void setReg64(int regno, reg64 val, int cpuIdx) {
    getCpu(cpuIdx)->setReg64(regno, val);
  }
  static inline void setReg32(int regno, reg32 val, int cpuIdx) {
    getCpu(cpuIdx)->setReg32(regno, val);
  }

  static inline void incrementRip() { getCurrentCpu()->incrementRip(); }
  static inline void decrementRip() { getCurrentCpu()->decrementRip(); }
  static inline void resetRip() { getCurrentCpu()->resetRip(); }

  // returns CPU name used by Gdb
  static inline const char* getCpuName(int cpuIdx) {
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return gdbCPUs[cpuIdx].getName();
  }
  // total # of CPUs in the system
  static inline int getNumCpus() { return numCpu; }
  // total # of CPUs initialized for gdb use
  static inline int getNumCpusInitialized() { return numInitialized; }

  static void start() {
    if (!DBG::test(DBG::GDBEnable)) return;
    KASSERT1(numCpu > 0, numCpu);
    set_debug_traps(DBG::test(DBG::GDBAllStop));
    StdOut.outln("Waiting for Gdb connection");
    breakpoint();   // initiate connection to gdb
  }

  // sends IPI to other available cores
  static void sendIPIToAllOtherCores(int ipiNum) {
    for (int i = 0; i < numCpu; i++) {
      if (gdbCPUs[i].getCpuState() != cpuState::BREAKPOINT &&
          gdbCPUs[i].getCpuState() != cpuState::UNKNOWN) {
        sendIPI(i, ipiNum);
      }
    }
  }

  // semaphores to synchronize cores access to Gdb interrupt handlers
  static void P(int cpuIdx) {
    KASSERT1(cpuIdx >=0 && cpuIdx < numCpu && cpuIdx < numInitialized, cpuIdx);
    DBG::outln(DBG::GDBDebug, "enter P(", cpuIdx+1, ')');
    sem[cpuIdx].P();
    DBG::outln(DBG::GDBDebug, "leave P(", cpuIdx+1, ')');
  }
  static void V(int cpuIdx) {
    KASSERT1(cpuIdx >=0 && cpuIdx < numCpu && cpuIdx < numInitialized, cpuIdx);
    sem[cpuIdx].V();
    DBG::outln(DBG::GDBDebug, "V(", cpuIdx+1, ')');
  }

  static inline mword getUnwindDebugHookAddr() {
    return unWindDebugHookAddr;
  }

private:
  // sends an IPI to a specified CPU core
  static void sendIPI(int cpuIdx, int ipiNum) {
    DBG::outln(DBG::GDBDebug, "sending IPI ", FmtHex(ipiNum),
        " from ", Processor::getApicID() + 1, " to core: ", cpuIdx+1);
    mword err = Processor::sendIPI(cpuIdx, ipiNum);
    KASSERT1(!err, FmtHex(err));
  }

  // read virtual address of _Unwind_DebugHook symbol
  static mword findUnwindDebugHookAddr() {
    ELFIO::elfio elfReader;
    File* kernelElf = kernelFS.find("kernel.sys.debug")->second;
    KASSERT0(elfReader.load(kernelElf->startptr(), kernelElf->size()));
    KASSERT1(elfReader.get_class() == ELFCLASS64, elfReader.get_class());
    ELFIO::Elf_Half sec_num = elfReader.sections.size();
    for (int i = 0; i < sec_num; ++i) {
      ELFIO::section* psec = elfReader.sections[i];
      if (psec->get_type() == SHT_SYMTAB) {
        const ELFIO::symbol_section_accessor symbols(elfReader, psec);
        for (unsigned int j = 0; j < symbols.get_symbols_num(); ++j) {
          kstring name;
          ELFIO::Elf64_Addr value;
          ELFIO::Elf_Xword size;
          unsigned char bind;
          unsigned char type;
          ELFIO::Elf_Half sec_idx;
          unsigned char other;
          symbols.get_symbol(j, name, value, size, bind, type, sec_idx, other);
          if (name == "_Unwind_DebugHook") {
            return (mword)value;
          }
        }
      }
    }
    return 0;
  }

  Gdb() = delete;                         // no creation
  Gdb(const Gdb&) = delete;               // no copy
  Gdb& operator=(const Gdb&) = delete;    // no assignment

  static GdbCpu* gdbCPUs;
  static int numCpu;
  static int numInitialized;
  static int enumIdx;
  static NonBlockSemaphore* sem;          // split binary semaphore
  static SpinLock mutex;
  static mword unWindDebugHookAddr;
};

#endif // Gdb_h_
