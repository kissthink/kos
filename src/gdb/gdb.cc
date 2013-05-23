#include "gdb.h"
#include "GdbCpu.h"
#include "Semaphore.h"
#include "util/Debug.h"
#include "util/SpinLock.h"
#include "mach/CPU.h"
#include "mach/APIC.h"
#include "mach/Memory.h"
#include "mach/Processor.h"

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
  enumIdx(0), sem(0), vContActionQueue(0), vContActionReply(nullptr) {}

void GDB::init(int numCpu, Processor* processorTable) {
  KASSERT0(numCpu);
  this->numCpu = numCpu;
  cpuStates = new GdbCpuState[numCpu];
  this->processorTable = processorTable;
  DBG::outln(DBG::GDBDebug, "Processor table ", FmtHex(processorTable));
  sem = new Semaphore[numCpu];
  vContActionQueue = new EmbeddedQueue<VContAction>[numCpu];
  for (int i = 0; i < numCpu; i++) {
      cpuStates[i].setCpuId(i);
  }

  setupGDB(0);        // install CpuStates needed for GDB in BSP
  DBG::outln(DBG::GDBDebug, "GDB state initialized for ", numCpu, " cores");
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
  DBG::outln(DBG::GDBDebug, "setup cpu: ", cpuIdx+1);
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
  StdOut.outln("Waiting for GDB connection");
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
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
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
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
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
      DBG::outln(DBG::GDBDebug, "sending ipi 0x1 from ", curCpuIdx+1, " to core: ", i+1);
      KASSERT0(lapic);
      mword err = lapic->sendIPI(i, 0x1);
      KASSERT1(!err, FmtHex(err));
    }
  }
}

void GDB::P(int cpuIdx) {
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT1(cpuIdx < numInitialized, cpuIdx);
  DBG::outln(DBG::GDBDebug, "enter P(", cpuIdx+1, ')');
  sem[cpuIdx].P();
  DBG::outln(DBG::GDBDebug, "leave P(", cpuIdx+1, ')');
}

void GDB::V(int cpuIdx) {
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT1(cpuIdx < numInitialized, cpuIdx);
  DBG::outln(DBG::GDBDebug, "enter V(", cpuIdx+1, ')');
  sem[cpuIdx].V();
  DBG::outln(DBG::GDBDebug, "leave V(", cpuIdx+1, ')');
}

void GDB::addVContAction(VContAction* action, int cpuIdx) {
  ScopedLockISR<> so(mutex);
  KASSERT0(action);
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT1(cpuIdx < numInitialized, cpuIdx);
  vContActionQueue[cpuIdx].push(action);
}

void GDB::addVContAction(VContAction* action) {
  ScopedLockISR<> so(mutex);
  KASSERT0(action);
  vContActionQueue[Processor::getApicID()].push(action);
}

bool GDB::isEmptyVContActionQueue() const {
  ScopedLockISR<> so(mutex);
  return vContActionQueue[Processor::getApicID()].empty();
}

bool GDB::isEmptyVContActionQueue(int cpuIdx) const {
  ScopedLockISR<> so(mutex);
  KASSERT1(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
  KASSERT1(cpuIdx < numInitialized, cpuIdx);
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
  KASSERT1(vContActionReply == 0, vContActionReply);
  vContActionReply = new VContActionReply(
      Processor::getApicID(),
      signal
  );
}

VContActionReply* GDB::removeVContActionReply() {
  ScopedLockISR<> so(mutex);
  KASSERT0(vContActionReply);
  VContActionReply* reply = vContActionReply;
  vContActionReply = 0;
  return reply;
}
