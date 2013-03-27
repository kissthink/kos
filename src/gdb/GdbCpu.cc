#include "GdbCpu.h"

using namespace gdb;

GdbCpuState::GdbCpuState()
: RAX(0), RBX(0), RCX(0), RDX(0), RSI(0), RDI(0)
, RBP(0), RSP(0), R8(0), R9(0), R10(0), R11(0), R12(0)
, R13(0), R14(0), R15(0), RIP(0), EFLAGS(0), CS(0), SS(0)
, DS(0), ES(0), FS(0), GS(0)
, gdbErrorCode(0)
{
    // initialize stack to 0
    for (unsigned int i = 0; i < 10000/sizeof(uint64_t); i++) {
        stack[i] = 0;
    }
    for (int i = 0; i < num64Registers; i++) {
        buffer64[i] = 0;
    }
    for (int i = 0; i < num32Registers; i++) {
        buffer32[i] = 0;
    }
}

int GdbCpuState::getNum64Registers() const {
    return num64Registers;
}

int GdbCpuState::getNum32Registers() const {
    return num32Registers;
}

/*
 * returns top of the stack
 */
uint64_t* GdbCpuState::stackPtr() {
    return stack + 10000/sizeof(uint64_t);
}

uint64_t* GdbCpuState::getBuffer64() {
    return buffer64;
}

uint32_t* GdbCpuState::getBuffer32() {
    return buffer32;
}

void GdbCpuState::saveToBuffer() {
    // save 64 bit registers
    buffer64[0] = RAX;
    buffer64[1] = RBX;
    buffer64[2] = RCX;
    buffer64[3] = RDX;
    buffer64[4] = RSI;
    buffer64[5] = RDI;
    buffer64[6] = RBP;
    buffer64[7] = RSP;
    buffer64[8] = R8;
    buffer64[9] = R9;
    buffer64[10] = R10;
    buffer64[11] = R11;
    buffer64[12] = R12;
    buffer64[13] = R13;
    buffer64[14] = R14;
    buffer64[15] = R15;
    buffer64[16] = RIP;

    // save 32 bit registers
    buffer32[0] = EFLAGS;
    buffer32[1] = CS;
    buffer32[2] = SS;
    buffer32[3] = DS;
    buffer32[4] = ES;
    buffer32[5] = FS;
    buffer32[6] = GS;
}

void GdbCpuState::saveRegisters() {
    saveRegisters1();
    saveRegisters2();
    saveToBuffer();
}

void GdbCpuState::saveRegistersWithErrorCode() {
    saveRegisters1();
    saveErrorCode();
    saveRegisters2();
    saveToBuffer();
}

reg64 GdbCpuState::getEFLAGS() {
    return EFLAGS;
}

reg64 GdbCpuState::getRIP() {
    return RIP;
}

void GdbCpuState::setRegister64(int regno, uint64_t val) {
    buffer64[regno] = val;
    switch (regno) {
        case registers::RAX:
            RAX = val; break;
        case registers::RBX:
            RBX = val; break;
        case registers::RCX:
            RCX = val; break;
        case registers::RDX:
            RDX = val; break;
        case registers::RSI:
            RSI = val; break;
        case registers::RDI:
            RDI = val; break;
        case registers::RBP:
            RBP = val; break;
        case registers::RSP:
            RSP = val; break;
        case registers::R8:
            R8 = val; break;
        case registers::R9:
            R9 = val; break;
        case registers::R10:
            R10 = val; break;
        case registers::R11:
            R11 = val; break;
        case registers::R12:
            R12 = val; break;
        case registers::R13:
            R13 = val; break;
        case registers::R14:
            R14 = val; break;
        case registers::R15:
            R15 = val; break;
        case registers::RIP:
            RIP = val; break;
    }
}

void GdbCpuState::setRegister32(int regno, uint32_t val) {
    buffer32[regno] = val;
    switch (regno) {
        case registers::EFLAGS:
            EFLAGS = val; break;
        case registers::CS:
            CS = val; break;
        case registers::SS:
            SS = val; break;
        case registers::DS:
            DS = val; break;
        case registers::ES:
            ES = val; break;
        case registers::FS:
            FS = val; break;
        case registers::GS:
            GS = val; break;
    }
}

uint64_t* GdbCpuState::getRegisterPtr64(int regno) {
    return &buffer64[regno];
}

uint32_t* GdbCpuState::getRegisterPtr32(int regno) {
    return &buffer32[regno];
}
