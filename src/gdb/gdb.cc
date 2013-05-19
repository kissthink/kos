#include "gdb.h"
#include "GdbCpu.h"
#include "Semaphore.h"
#include "util/Log.h"
#include "mach/platform.h"
#include "mach/CPU.h"
#include "mach/APIC.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "util/SpinLock.h"

using namespace gdb;

extern void set_debug_traps();      // from x86_64-stub.cc
extern void breakpoint();           // from x86_64-stub.cc

SpinLock mutex;                     // mutex for GDB object
                                    // declared static so it can be used inside const methods

GDB& GDB::getInstance() {
  static GDB gdb;
  return gdb;
}

GDB::GDB()
: cpuStates(0), processorTable(0)
, numCpu(0), numInitialized(0), enumIdx(0)
, sem(0)
, vContActionQueue(0), vContActionReply(0)
{}

void GDB::init(int numCpu, Processor* processorTable) {
  KASSERT(numCpu > 0, numCpu);
  this->numCpu = numCpu;
  cpuStates = new GdbCpuState[numCpu];
  this->processorTable = processorTable;
  kcdbg << "Processor table " << FmtHex(processorTable) << "\n";
  sem = new Semaphore[numCpu];
  vContActionQueue = new EmbeddedQueue<VContAction>[numCpu];
  for (int i = 0; i < numCpu; i++) {
      cpuStates[i].setCpuId(i);
  }

  setupGDB(0);        // install CpuStates needed for GDB in BSP
  kcdbg << "GDB state initialized for " << numCpu << " cores.\n";
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
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  return &cpuStates[cpuIdx];
}

void GDB::setupGDB(int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  processorTable[cpuIdx].initGdbCpuStates(&cpuStates[cpuIdx]);
  kcdbg << "setup cpu: " << cpuIdx+1 << "\n";
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
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  return cpuStates[cpuIdx].getId();
}

void GDB::startGdb() {
  KASSERT(numCpu > 0, numCpu);
  set_debug_traps();
  kcout << "Waiting for GDB connection\n";
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

void GDB::setGCpu(int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  asm volatile("mov %0, %%fs:64" :: "r"(&cpuStates[cpuIdx]) : "memory");
}

GdbCpuState* GDB::getGCpu() const {
  ScopedLockISR<> so(mutex);
  mword x;
  asm volatile("mov %%fs:64, %0" : "=r"(x));
  if (x) return reinterpret_cast<GdbCpuState *>(x);
  return _getCurrentCpuState();
}

void GDB::setCCpu(int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  asm volatile("mov %0, %%fs:56" :: "r"(&cpuStates[cpuIdx]) : "memory");
}

GdbCpuState* GDB::getCCpu() const {
  ScopedLockISR<> so(mutex);
  mword x;
  asm volatile("mov %%fs:56, %0" : "=r"(x));
  if (x) return reinterpret_cast<GdbCpuState *>(x);
  return _getCurrentCpuState();
}

void GDB::sendIPIToAllOtherCores() const {
  int curCpuIdx = Processor::getApicID();
  for (int i = 0; i < numCpu; i++) {
    if (cpuStates[i].getCpuState() != cpuState::BREAKPOINT &&
        cpuStates[i].getCpuState() != cpuState::UNKNOWN) {
      LAPIC* lapic = (LAPIC *) lapicAddr;
      kcdbg << "sending ipi 0x1 from " << curCpuIdx+1 << " to core: " << i+1 << "\n";
      KASSERT(lapic != 0, "LAPIC is NULL");
      mword err = lapic->sendIPI(i, 0x1);
      KASSERT(err == 0, FmtHex(err));
    }
  }
}

void GDB::P(int cpuIdx) {
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT(cpuIdx < numInitialized, cpuIdx);
  kcdbg << "enter P(" << cpuIdx+1 << ")\n";
  sem[cpuIdx].P();
  kcdbg << "leave P(" << cpuIdx+1 << ")\n";
}

void GDB::V(int cpuIdx) {
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT(cpuIdx < numInitialized, cpuIdx);
  kcdbg << "enter V(" << cpuIdx+1 << ")\n";
  sem[cpuIdx].V();
  kcdbg << "leave V(" << cpuIdx+1 << ")\n";
}

void GDB::addVContAction(VContAction* action, int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT(action != 0, action);
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT(cpuIdx < numInitialized, cpuIdx);
  vContActionQueue[cpuIdx].push(action);
}

void GDB::addVContAction(VContAction* action) {
  ScopedLockISR<> so(mutex);
  KASSERT(action != 0, action);
  vContActionQueue[Processor::getApicID()].push(action);
}

bool GDB::isEmptyVContActionQueue() const {
  ScopedLockISR<> so(mutex);
  return vContActionQueue[Processor::getApicID()].empty();
}

bool GDB::isEmptyVContActionQueue(int cpuIdx) const {
  ScopedLockISR<> so(mutex);
  KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT(cpuIdx < numInitialized, cpuIdx);
  return vContActionQueue[cpuIdx].empty();
}

VContAction* GDB::nextVContAction() {
  ScopedLockISR<> so(mutex);
  return vContActionQueue[Processor::getApicID()].front();
}

const VContAction* GDB::nextVContAction() const {
  ScopedLockISR<> so(mutex);
  return vContActionQueue[Processor::getApicID()].front();
}

void GDB::popVContAction() {
  ScopedLockISR<> so(mutex);
  vContActionQueue[Processor::getApicID()].pop();
}

void GDB::setVContActionReply(int signal) {
  ScopedLockISR<> so(mutex);
  KASSERT(vContActionReply == 0, "Pending vCont stop reply.");
  vContActionReply = new VContActionReply(
      Processor::getApicID(),
      signal
  );
}

VContActionReply* GDB::removeVContActionReply() {
  ScopedLockISR<> so(mutex);
  KASSERT(vContActionReply, "vCont stop reply not set.");
  VContActionReply* reply = vContActionReply;
  vContActionReply = 0;
  return reply;
}
