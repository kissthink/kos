#ifndef Gdb_h_
#define Gdb_h_ 1

#include "gdb/GdbCpu.h"
#include "gdb/NonBlockSemaphore.h"
#include "mach/CPU.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "kern/Debug.h"
#include "ipc/SpinLock.h"

extern void set_debug_traps(bool);
extern void breakpoint();

class Gdb {
public:
  static Gdb& getInstance() {
    static Gdb gdb;
    return gdb;
  }
  // initialize internal data structures & variables
  void init(int numCPU) {
    if (!DBG::test(DBG::GDBEnable)) return;
    KASSERT0(numCPU);
    numCpu = numCPU;
    cpuStates = new GdbCpu[numCpu];
    sem = new NonBlockSemaphore[numCpu];
    for (int i = 0; i < numCpu; i++) {
      cpuStates[i].setCpuId(i);
    }
    DBG::outln(DBG::GDBDebug, "Gdb state initialized for ", numCpu, " cores");
  }

  // returns CPU states (locked version)
  GdbCpu* getCurrentCpuState() const {
    ScopedLock<> so(mutex);
    return _getCurrentCpuState();
  }
  // returns CPU state for the current CPU
  GdbCpu* getCpuState(int cpuIdx) {
    ScopedLock<> so(mutex);
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return &cpuStates[cpuIdx];
  }

  // gdb breakpoints can be set after calling this method.
  GdbCpu* setupGdb(int cpuIdx) {
    if (!DBG::test(DBG::GDBEnable)) return nullptr;
    ScopedLock<> so(mutex);
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    DBG::outln(DBG::GDBDebug, "setup cpu ", cpuIdx+1);
    numInitialized = cpuIdx+1;
    cpuStates[cpuIdx].setCpuState(cpuState::RUNNING);
    return &cpuStates[cpuIdx];
  }

  // enumerate CPUs
  void startEnumerate() {
    ScopedLock<> so(mutex);
    enumIdx = 0;
  }
  GdbCpu* next() {
    ScopedLock<> so(mutex);
    if (enumIdx < numCpu) return &cpuStates[enumIdx++];
    return nullptr;
  }

  // access registers
  // returns a buffer storing 64-bit registers
  char* getRegs64() const {
    ScopedLock<> so(mutex);
    GdbCpu* state = _getCurrentCpuState();
    return reinterpret_cast<char *>(state->getRegs64());
  }
  // a buffer storing 32-bit registers
  char* getRegs32() const {
    ScopedLock<> so(mutex);
    GdbCpu* state = _getCurrentCpuState();
    return reinterpret_cast<char *>(state->getRegs32());
  }

  // returns CPU name used by Gdb
  const char* getCpuId(int cpuIdx) {
    ScopedLock<> so(mutex);
    KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return cpuStates[cpuIdx].getId();
  }
  // total # of CPUs in the system
  int getNumCpus() const {
    ScopedLock<> so(mutex);
    return numCpu;
  }
  // total # of CPUs initialized for gdb use
  int getNumCpusInitialized() const {
    ScopedLock<> so(mutex);
    return numInitialized;
  }

  // starts initial breakpoint
  void start() {
    if (!DBG::test(DBG::GDBEnable)) return;
    set_debug_traps(DBG::test(DBG::GDBAllStop));
    StdOut.outln("Waiting for Gdb connection");
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
    DBG::outln(DBG::GDBDebug, "enter P(", cpuIdx+1, ')');
    sem[cpuIdx].P();
    DBG::outln(DBG::GDBDebug, "leave P(", cpuIdx+1, ')');
  }
  void V(int cpuIdx) {
    KASSERT1(cpuIdx >=0 && cpuIdx < numCpu && cpuIdx < numInitialized, cpuIdx);
    sem[cpuIdx].V();
    DBG::outln(DBG::GDBDebug, "V(", cpuIdx+1, ')');
  }


private:
  // returns CPU state (actual implementation, unlocked)
  GdbCpu* _getCurrentCpuState() const {
    return Processor::getGdbCpu();
  }

  // sends an IPI to a specified CPU core
  void sendIPI(int cpuIdx, int ipiNum) const {
    DBG::outln(DBG::GDBDebug, "sending IPI ", FmtHex(ipiNum),
        " from ", Processor::getApicID() + 1, " to core: ", cpuIdx+1);
    mword err = Processor::sendIPI(cpuIdx, ipiNum);
    KASSERT1(!err, FmtHex(err));
  }

  Gdb(): cpuStates(nullptr), numCpu(0), numInitialized(0), enumIdx(0), sem(nullptr) {}
  GdbCpu* cpuStates;
  int numCpu;
  int numInitialized;
  int enumIdx;
  NonBlockSemaphore* sem;                           // split binary semaphore
  mutable SpinLock mutex;
};

#endif // Gdb_h_
