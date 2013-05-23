#include "util/Debug.h"
#include "gdb/GdbCpu.h"
#include <cstring>

#undef __STRICT_ANSI__
#include <cstdio>

using namespace gdb;
using namespace std;

namespace gdb {
  namespace cpuState {
    const char* cpuStateStr[] = { "Unknown",
                                  "Halted",
                                  "Paused",
                                  "Breakpoint",
                                  "Running" };
  }
}

GdbCpuState::GdbCpuState()
: gdbErrorCode(0)
, state(cpuState::UNKNOWN)
, ripDecremented(false) {
  memset(cpuId, 0, sizeof(char) * 20);
  memset(stack, 0, bufferSize);
  memset(reg64Buffer, 0, numRegs64 * sizeof(reg64));
  memset(reg32Buffer, 0, numRegs32 * sizeof(reg32));
}

void GdbCpuState::_setCpuIdStr() {
  memset(cpuId, 0, sizeof(char) * 20);
  snprintf(cpuId, 20, "Core %d [%s]", cpuIndex+1, cpuState::cpuStateStr[state]);
}

void GdbCpuState::setCpuIdStr() {
  ScopedLockISR<> so(mutex);
  _setCpuIdStr();
}

void GdbCpuState::setCpuId(int cpuIndex) {
  ScopedLockISR<> so(mutex);
  this->cpuIndex = cpuIndex;
  _setCpuIdStr();
  char buf2[10];
  snprintf(buf2, 10, "m%d", cpuIndex+1);
  cpuInfo = buf2;
}

void GdbCpuState::setCpuState(cpuState::cpuStateEnum newState) {
  ScopedLockISR<> so(mutex);
  state = newState;
  _setCpuIdStr();
}

cpuState::cpuStateEnum GdbCpuState::getCpuState() {
  ScopedLockISR<> so(mutex);
  return state;
}

uint64_t* GdbCpuState::stackPtr() {
  ScopedLockISR<> so(mutex);
  return stack + bufferSize/sizeof(uint64_t);
}

reg64* GdbCpuState::getRegs64() {
  ScopedLockISR<> so(mutex);
  return reg64Buffer;
}

reg32* GdbCpuState::getRegs32() {
  ScopedLockISR<> so(mutex);
  return reg32Buffer;
}

reg64* GdbCpuState::getReg64(int regno) {
  ScopedLockISR<> so(mutex);
  return &reg64Buffer[regno];
}

reg32* GdbCpuState::getReg32(int regno) {
  ScopedLockISR<> so(mutex);
  return &reg32Buffer[regno];
}

void GdbCpuState::setReg64(int regno, reg64 val) {
  ScopedLockISR<> so(mutex);
  reg64Buffer[regno] = val;
}

void GdbCpuState::setReg32(int regno, reg32 val) {
  ScopedLockISR<> so(mutex);
  reg32Buffer[regno] = val;
}

void GdbCpuState::decrementRip() {
  ScopedLockISR<> so(mutex);
  if (!ripDecremented) {
    reg64Buffer[registers::RIP] -= 1;
    ripDecremented = true;
  }
}

void GdbCpuState::incrementRip() {
  ScopedLockISR<> so(mutex);
  if (ripDecremented) {
    reg64Buffer[registers::RIP] += 1;
    ripDecremented = false;
    DBG::outlnISR(DBG::GDBDebug, "rip incremented");
  }
}

void GdbCpuState::resetRip() {
  ScopedLockISR<> so(mutex);
  ripDecremented = false;
}
