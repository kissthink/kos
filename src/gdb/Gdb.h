#ifndef Gdb_h_
#define Gdb_h_ 1

#include "gdb/GdbCpu.h"
#include "gdb/NonBlockSemaphore.h"
#include "mach/APIC.h"
#include "mach/CPU.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "util/Debug.h"
#include "util/SpinLock.h"

extern void set_debug_traps(bool);
extern void breakpoint();

class Gdb {
public:
  static Gdb& getInstance() {
    static Gdb gdb;
    return gdb;
  }
  // initialize internal data structures & variables
  void init(int numCPU, Processor* procTable) {
    KASSERT0(numCPU);
    numCpu = numCPU;
    cpuStates = new GdbCpuState[numCpu];
    processorTable = procTable;
    DBG::outlnISR(DBG::GDBDebug, "Processor table ", FmtHex(processorTable));
    sem = new NonBlockSemaphore[numCpu];
    for (int i = 0; i < numCpu; i++) {
      cpuStates[i].setCpuId(i);
    }
    setupGdb(0);
    DBG::outlnISR(DBG::GDBDebug, "Gdb state initialized for ", numCpu, " cores");
  }

  // returns CPU states (locked version)
  GdbCpuState* getCurrentCpuState() const {
    ScopedLockISR<> so(mutex);
    return _getCurrentCpuState();
  }
  // returns CPU state for the current CPU
  GdbCpuState* getCpuState(int cpuIdx) {
    ScopedLockISR<> so(mutex);
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return &cpuStates[cpuIdx];
  }

  // gdb breakpoints can be set after calling this method.
  void setupGdb(int cpuIdx) {
    ScopedLockISR<> so(mutex);
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    processorTable[cpuIdx].initGdbCpuStates(&cpuStates[cpuIdx]);
    DBG::outlnISR(DBG::GDBDebug, "setup cpu ", cpuIdx+1);
    numInitialized = cpuIdx+1;
    cpuStates[cpuIdx].setCpuState(cpuState::RUNNING);
  }

  // enumerate CPUs
  void startEnumerate() {
    ScopedLockISR<> so(mutex);
    enumIdx = 0;
  }
  GdbCpuState* next() {
    ScopedLockISR<> so(mutex);
    if (enumIdx < numCpu) return &cpuStates[enumIdx++];
    return nullptr;
  }

  // access registers
  // returns a buffer storing 64-bit registers
  char* getRegs64() const {
    ScopedLockISR<> so(mutex);
    GdbCpuState* state = _getCurrentCpuState();
    return reinterpret_cast<char *>(state->getRegs64());
  }
  // a buffer storing 32-bit registers
  char* getRegs32() const {
    ScopedLockISR<> so(mutex);
    GdbCpuState* state = _getCurrentCpuState();
    return reinterpret_cast<char *>(state->getRegs32());
  }

  // returns CPU name used by Gdb
  const char* getCpuId(int cpuIdx) {
    ScopedLockISR<> so(mutex);
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return cpuStates[cpuIdx].getId();
  }
  // total # of CPUs in the system
  int getNumCpus() const {
    ScopedLockISR<> so(mutex);
    return numCpu;
  }
  // total # of CPUs initialized for gdb use
  int getNumCpusInitialized() const {
    ScopedLockISR<> so(mutex);
    return numInitialized;
  }

  // starts initial breakpoint
  void startGdb(bool allstop) {
    KASSERT0(numCpu);
    set_debug_traps(allstop);
    StdOut.outlnISR("Waiting for Gdb connection");
    cpuStates[0].setCpuState(cpuState::RUNNING);
    breakpoint();
  }

  // sends IPI to other available cores
  void sendIPIToAllOtherCores(int ipiNum) const {
    for (int i = 0; i < numCpu; i++) {
      if (cpuStates[i].getCpuState() != cpuState::BREAKPOINT &&
          cpuStates[i].getCpuState() != cpuState::UNKNOWN) {
        sendIPI(i, ipiNum);
      }
    }
  }

  // semaphores to synchronize cores access to Gdb interrupt handlers
  void P(int cpuIdx) {
    KASSERT1(cpuIdx >=0 && cpuIdx < numCpu && cpuIdx < numInitialized, cpuIdx);
    DBG::outlnISR(DBG::GDBDebug, "enter P(", cpuIdx+1, ')');
    sem[cpuIdx].P();
    DBG::outlnISR(DBG::GDBDebug, "leave P(", cpuIdx+1, ')');
  }
  void V(int cpuIdx) {
    KASSERT1(cpuIdx >=0 && cpuIdx < numCpu && cpuIdx < numInitialized, cpuIdx);
    sem[cpuIdx].V();
    DBG::outlnISR(DBG::GDBDebug, "V(", cpuIdx+1, ')');
  }


private:
  // returns CPU state (actual implementation, unlocked)
  GdbCpuState* _getCurrentCpuState() const {
    mword x;
    asm volatile("mov %%fs:48, %0" : "=r"(x));
    return reinterpret_cast<GdbCpuState *>(x);
  }

  // sends an IPI to a specified CPU core
  void sendIPI(int cpuIdx, int ipiNum) const {
    LAPIC* lapic = (LAPIC *) lapicAddr;
    DBG::outlnISR(DBG::GDBDebug, "sending IPI ", FmtHex(ipiNum),
        " from ", Processor::getApicID()+1, " to core: ", cpuIdx+1);
    KASSERT0(lapic);
    mword err = lapic->sendIPI(cpuIdx, ipiNum);
    KASSERT1(!err, FmtHex(err));
  }

  Gdb(): cpuStates(nullptr), processorTable(nullptr)
  , numCpu(0), numInitialized(0), enumIdx(0), sem(nullptr) {}
  GdbCpuState* cpuStates;
  Processor* processorTable;
  int numCpu;
  int numInitialized;
  int enumIdx;
  NonBlockSemaphore* sem;                           // split binary semaphore
  mutable SpinLock mutex;
};

#endif // Gdb_h_
