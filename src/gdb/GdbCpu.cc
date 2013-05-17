#include "GdbCpu.h"
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
{
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
    ScopedLock<> so(mutex);
    _setCpuIdStr();
}

void GdbCpuState::setCpuId(int cpuIndex) {
    ScopedLock<> so(mutex);

    this->cpuIndex = cpuIndex;
    _setCpuIdStr();
    char buf2[10];
    snprintf(buf2, 10, "m%d", cpuIndex+1);
    cpuInfo = buf2;
}

void GdbCpuState::setCpuState(cpuState::cpuStateEnum newState) {
    ScopedLock<> so(mutex);

    state = newState;
    _setCpuIdStr();
}

cpuState::cpuStateEnum GdbCpuState::getCpuState() {
    ScopedLock<> so(mutex);
    return state;
}

/*
 * returns top of the stack
 */
uint64_t* GdbCpuState::stackPtr() {
    ScopedLock<> so(mutex);
    return stack + bufferSize/sizeof(uint64_t);
}

// void GdbCpuState::restoreRegisters() in gdb_asm_functions.S

reg64* GdbCpuState::getRegs64() {
    ScopedLock<> so(mutex);
    return reg64Buffer;
}

reg32* GdbCpuState::getRegs32() {
    ScopedLock<> so(mutex);
    return reg32Buffer;
}

reg64* GdbCpuState::getReg64(int regno) {
    ScopedLock<> so(mutex);
    return &reg64Buffer[regno];
}

reg32* GdbCpuState::getReg32(int regno) {
    ScopedLock<> so(mutex);
    return &reg32Buffer[regno];
}

void GdbCpuState::setReg64(int regno, reg64 val) {
    ScopedLock<> so(mutex);
    reg64Buffer[regno] = val;
}

void GdbCpuState::setReg32(int regno, reg32 val) {
    ScopedLock<> so(mutex);
    reg32Buffer[regno] = val;
}
