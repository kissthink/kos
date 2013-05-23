#include "gdb.h"
#include "GdbCpu.h"
#include "Semaphore.h"
#include "util/Debug.h"
#include "util/SpinLock.h"
#include "mach/CPU.h"
#include "mach/APIC.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "util/Debug.h"
#include "util/SpinLock.h"

using namespace gdb;

extern void set_debug_traps(bool);  // from x86_64-stub.cc
extern void breakpoint();           // from x86_64-stub.cc

SpinLock mutex;                     // mutex for GDB object
                                    // declared static so it can be used inside const methods

GDB& GDB::getInstance() {
  static GDB gdb;
  return gdb;
}

GDB::GDB() : cpuStates(0), processorTable(0) , numCpu(0), numInitialized(0),
  enumIdx(0), sem(0) {}

void GDB::init(int numCpu, Processor* processorTable) {
  KASSERT0(numCpu);
  this->numCpu = numCpu;
  cpuStates = new GdbCpuState[numCpu];
  this->processorTable = processorTable;
  DBG::outlnISR(DBG::GDBDebug, "Processor table ", FmtHex(processorTable));
  sem = new Semaphore[numCpu];
  for (int i = 0; i < numCpu; i++) {
      cpuStates[i].setCpuId(i);
  }

  setupGDB(0);        // install CpuStates needed for GDB in BSP
  DBG::outlnISR(DBG::GDBDebug, "GDB state initialized for ", numCpu, " cores");
}

GdbCpuState* GDB::getCurrentCpuState() const {
  ScopedLockISR<> so(mutex);
  return _getCurrentCpuState();
}

GdbCpuState* GDB::_getCurrentCpuState() const {
  mword x;
  asm volatile("mov %%fs:48, %0" : "=r"(x));
  return reinterpret_cast<GdbCpuState *>(x);
}

GdbCpuState* GDB::getCpuState(int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  return &cpuStates[cpuIdx];
}

void GDB::setupGDB(int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  processorTable[cpuIdx].initGdbCpuStates(&cpuStates[cpuIdx]);
  DBG::outlnISR(DBG::GDBDebug, "setup cpu: ", cpuIdx+1);
  numInitialized = cpuIdx+1;
  cpuStates[cpuIdx].setCpuState(cpuState::RUNNING);
}

void GDB::startEnumerate() {
  ScopedLockISR<> so(mutex);
  enumIdx = 0;
}

GdbCpuState* GDB::next() {
  ScopedLockISR<> so(mutex);
  if (enumIdx < numCpu) return &cpuStates[enumIdx++];
  return NULL;
}

char* GDB::getRegs64() const {
  ScopedLockISR<> so(mutex);
  GdbCpuState* curCpuState = _getCurrentCpuState();
  return reinterpret_cast<char *>(curCpuState->getRegs64());
}

char* GDB::getRegs32() const {
  ScopedLockISR<> so(mutex);
  GdbCpuState* curCpuState = _getCurrentCpuState();
  return reinterpret_cast<char *>(curCpuState->getRegs32());
}

const char* GDB::getCpuId(int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  return cpuStates[cpuIdx].getId();
}

void GDB::startGdb(bool allstop) {
  KASSERT0(numCpu);
  set_debug_traps(allstop);
  StdOut.outlnISR("Waiting for GDB connection");
  cpuStates[0].setCpuState(cpuState::RUNNING);
  breakpoint();
}

int GDB::getNumCpus() const {
  ScopedLockISR<> so(mutex);
  return numCpu;
}

int GDB::getNumCpusInitialized() const {
  ScopedLockISR<> so(mutex);
  return numInitialized;
}

void GDB::sendIPIToAllOtherCores() const {
  int curCpuIdx = Processor::getApicID();
  for (int i = 0; i < numCpu; i++) {
    if (cpuStates[i].getCpuState() != cpuState::BREAKPOINT &&
        cpuStates[i].getCpuState() != cpuState::UNKNOWN) {
      LAPIC* lapic = (LAPIC *) lapicAddr;
      DBG::outlnISR(DBG::GDBDebug, "sending ipi 0x1 from ", curCpuIdx+1, " to core: ", i+1);
      KASSERT0(lapic);
      mword err = lapic->sendIPI(i, 0x1);
      KASSERT1(!err, FmtHex(err));
    }
  }
}

void GDB::P(int cpuIdx) {
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT1(cpuIdx < numInitialized, cpuIdx);
  DBG::outlnISR(DBG::GDBDebug, "enter P(", cpuIdx+1, ')');
  sem[cpuIdx].P();
  DBG::outlnISR(DBG::GDBDebug, "leave P(", cpuIdx+1, ')');
}

void GDB::V(int cpuIdx) {
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT1(cpuIdx < numInitialized, cpuIdx);
  DBG::outlnISR(DBG::GDBDebug, "enter V(", cpuIdx+1, ')');
  sem[cpuIdx].V();
  DBG::outlnISR(DBG::GDBDebug, "leave V(", cpuIdx+1, ')');
}

void GDB::sendINT1(int cpuIdx) {
  LAPIC* lapic = (LAPIC *) lapicAddr;
  DBG::outlnISR(DBG::GDBDebug, "sending ipi 0x1 from ", Processor::getApicID()+1, " to core: ", cpuIdx+1);
  KASSERT0(lapic);
  mword err = lapic->sendIPI(cpuIdx, 0x1);
  KASSERT1(err == 0, FmtHex(err));
}