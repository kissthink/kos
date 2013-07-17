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

#define GdbInstance Gdb::getInstance()

class Gdb {
public:
  static Gdb& getInstance() {
    static Gdb gdb;
    return gdb;
  }
  void init(int numCPU, Processor* processorTable) {
    KASSERT1(numCPU > 0, numCPU);
    numCpu = numCPU;
    cpuStates = new GdbCpu[numCpu];
    sem = new NonBlockSemaphore[numCpu];
    for (int i = 0; i < numCpu; i++) {
      cpuStates[i].setCpuId(i);
      processorTable[i].setGdbCpu(&cpuStates[i]);
    }
    markGdbReady(0);
    DBG::outlnISR(DBG::GDBDebug, "Gdb state initialized for ", numCpu, " cores");
  }

  // gives pointer to asked GdbCpu object
  inline GdbCpu* getCurrentCpuState() {
    return Processor::getGdbCpu();
  }
  inline GdbCpu* getCpuState(int cpuIdx) {
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return &cpuStates[cpuIdx];
  }
  inline void setCpuState(cpuState::cpuStateEnum state) {
    Processor::getGdbCpu()->setCpuState(state);
  }

  // mark current CPU gdb debugging ready
  inline void markGdbReady(int cpuIdx) {
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    numInitialized = cpuIdx+1;
    cpuStates[cpuIdx].setCpuState(cpuState::RUNNING);
  }

  // enumerate CPUs
  inline void startEnumerate() {
    ScopedLockISR<> so(mutex);
    enumIdx = 0;
  }
  inline GdbCpu* next() {
    ScopedLockISR<> so(mutex);
    if (enumIdx < numCpu) return &cpuStates[enumIdx++];
    return nullptr;
  }

  // returns buffer storing gdb manipulated registers for current CPU
  inline char* getRegs64() {
    return reinterpret_cast<char *>(getCurrentCpuState()->getRegs64());
  }
  inline char* getRegs32() {
    return reinterpret_cast<char *>(getCurrentCpuState()->getRegs32());
  }
  inline reg64 getReg64(int regno) {
    return getCurrentCpuState()->getRegs64()[regno];
  }
  inline reg32 getReg32(int regno) {
    return getCurrentCpuState()->getRegs32()[regno];
  }
  inline void setReg64(int regno, reg64 val) {
    getCurrentCpuState()->setReg64(regno, val);
  }
  inline void setReg32(int regno, reg32 val) {
    getCurrentCpuState()->setReg32(regno, val);
  }

  inline void incrementRip() {
    getCurrentCpuState()->incrementRip();
  }

  // returns CPU name used by Gdb
  inline const char* getCpuName(int cpuIdx) const {
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return cpuStates[cpuIdx].getName();
  }
  // total # of CPUs in the system
  inline int getNumCpus() const {
    return numCpu;
  }
  // total # of CPUs initialized for gdb use
  inline int getNumCpusInitialized() const {
    return numInitialized;
  }

  void startGdb(bool allstop) {
    KASSERT1(numCpu > 0, numCpu);
    set_debug_traps(allstop);
    StdOut.outlnISR("Waiting for Gdb connection");
    breakpoint();   // initiate connection to gdb
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

  void disableLocalInterrupts() const {
    LAPIC* lapic = (LAPIC *) lapicAddr;
    lapic->maskInterrupts();
  }
  void enableLocalInterrupts() const {
    LAPIC* lapic = (LAPIC *) lapicAddr;
    lapic->unMaskInterrupts();
  }

private:
  // sends an IPI to a specified CPU core
  void sendIPI(int cpuIdx, int ipiNum) const {
    LAPIC* lapic = (LAPIC *) lapicAddr;
    DBG::outlnISR(DBG::GDBDebug, "sending IPI ", FmtHex(ipiNum),
        " from ", Processor::getApicID()+1, " to core: ", cpuIdx+1);
    KASSERT0(lapic);
    mword err = lapic->sendIPI(cpuIdx, ipiNum);
    KASSERT1(!err, FmtHex(err));
  }

  Gdb(): cpuStates(nullptr),numCpu(0),
    numInitialized(0), enumIdx(0), sem(nullptr) {}
  GdbCpu* cpuStates;
  int numCpu;
  int numInitialized;
  int enumIdx;
  NonBlockSemaphore* sem;                           // split binary semaphore
  mutable SpinLock mutex;
};

#endif // Gdb_h_
