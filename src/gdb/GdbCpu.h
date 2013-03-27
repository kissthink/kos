#ifndef GDB_CPU_H
#define GDB_CPU_H

#include <string>


namespace gdb {

typedef uint64_t reg64;
typedef uint32_t reg32;

namespace registers {
    enum reg_enum {
        RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
        R8, R9, R10, R11, R12, R13, R14, R15, RIP,
        EFLAGS = 0, CS, SS, DS, ES, FS, GS
    };
}

class GdbCpuState
{
    reg64  RAX;     // 0
    reg64  RBX;     // 8
    reg64  RCX;     // 16
    reg64  RDX;     // 24
    reg64  RSI;     // 32
    reg64  RDI;     // 40
    reg64  RBP;     // 48
    reg64  RSP;     // 56
    reg64  R8;      // 64
    reg64  R9;      // 72
    reg64  R10;     // 80
    reg64  R11;     // 88
    reg64  R12;     // 96
    reg64  R13;     // 104
    reg64  R14;     // 112
    reg64  R15;     // 120
    reg64  RIP;     // 128

    reg32  EFLAGS;  // 136
    reg32  CS;      // 140
    reg32  SS;      // 144
    reg32  DS;      // 148
    reg32  ES;      // 152
    reg32  FS;      // 156
    reg32  GS;      // 160

    int64_t gdbErrorCode;   // 164

    static const int num64Registers = 17;
    static const int num32Registers = 7;

    std::string cpuId;

    uint64_t stack[10000/sizeof(uint64_t)];
    uint64_t buffer64[num64Registers];
    uint32_t buffer32[num32Registers];

    void saveToBuffer();
    void saveRegisters1();
    void saveRegisters2();
    void saveErrorCode();

public:
    GdbCpuState();

    int getNum64Registers() const;
    int getNum32Registers() const;

    /**
     * Returns starting address of stack
     */
    uint64_t* stackPtr();

    /**
     * Returns from interrupt handler
     */
    void restoreRegisters();

    uint64_t* getBuffer64();
    uint32_t* getBuffer32();

    void saveRegisters();
    void saveRegistersWithErrorCode();

    reg64 getEFLAGS();
    reg64 getRIP();

    uint64_t* getRegisterPtr64(int regno);
    uint32_t* getRegisterPtr32(int regno);
    void setRegister64(int regno, uint64_t val);
    void setRegister32(int regno, uint32_t val);
};

};  // namespace gdb

#endif // GDB_CPU_H_
