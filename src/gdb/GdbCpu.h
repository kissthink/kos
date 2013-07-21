#ifndef GdbCpu_h_
#define GdbCpu_h_ 1

#include "kern/Debug.h"
#include "ipc/SpinLock.h"

#include <cstring>
#include <string>
#undef __STRICT_ANSI__ // get declaration of 'snprintf' 
#include <cstdio>

typedef uint64_t reg64;
typedef uint32_t reg32;

namespace registers {
  enum reg_enum {
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,     // registers are in order gdb front-end expects!
    R8, R9, R10, R11, R12, R13, R14, R15, RIP,  // it is preferred to not reorder
    EFLAGS = 0, CS, SS, DS, ES, FS, GS          // 32-bit registers
  };
}

/**
 * UNKNOWN - before GDB session for the CPU is initialized.
 * HALTED - when CPU has been halted.
 * PAUSED - when CPU is paused from pause instruction.
 * BREAKPOINT - currently being handled by GDB
 * RUNNING - CPU is running freely from GDB; GDB may not have
 * up to date register values.
 */
namespace cpuState {
  enum cpuStateEnum {
    UNKNOWN, HALTED, PAUSED, BREAKPOINT, RUNNING
  };
  static const char* cpuStateStr[] = { "Unknown",
                                       "Halted",
                                       "Paused",
                                       "Breakpoint",
                                       "Running" };
}

const int numRegs64  = 17;
const int numRegs32  = 7;
const int numRegs = numRegs64 + numRegs32;
const int bufferSize = (1<<20);
const int cpuNameLen = 20;
const int cpuIndexLen = 10;

class GdbCpu {
  // *do NOT change orders for these 4 variables*
  reg64 reg64Buffer[numRegs64];             // gdb_asm_functions.S requires this here
  reg32 reg32Buffer[numRegs32];             // registers are divided into 64/32-bits and stored in separate buffers and ordered as above
                                            // reg_enum enum because gdb front-end differentiates register sizes and expects above order.
                                            // separate buffers for 64/32-bit registers allow send/receive registers easily
  uint64_t stack[bufferSize];
  uint64_t* stackEnd = stack + bufferSize;
  char cpuName[cpuNameLen];                 // in format (Core %d) [State]
  int cpuIndex;
  cpuState::cpuStateEnum state;             // current state

  inline void setCpuName() {                // CPU names for human (info thread)
    snprintf(cpuName, cpuNameLen, "CPU %d [%s]", cpuIndex+1, cpuState::cpuStateStr[state]);
  }

public:
  GdbCpu(): state(cpuState::UNKNOWN) {
    memset(cpuName, 0, sizeof(char) * cpuNameLen);
    memset(stack, 0, sizeof(uint64_t) * bufferSize); 
    memset(reg64Buffer, 0, numRegs64 * sizeof(reg64));
    memset(reg32Buffer, 0, numRegs32 * sizeof(reg32));
  }

  // name/idx manipulation
  inline void setCpuId(int cpuIdx) {
    cpuIndex = cpuIdx;
    setCpuName();
  }
  inline void setCpuState(cpuState::cpuStateEnum newState) {
    state = newState;
    setCpuName();
  }
  inline cpuState::cpuStateEnum getCpuState() const {
    return state;
  }
  inline const char* getName() const {
    return cpuName;
  }

  inline uint64_t* stackPtr() const {   // per-CPU stack area reserved for gdb
    return stackEnd;
  }

  // manipulate stored registers
  // thread-safe as only called by its own CPU
  inline reg64* getRegs64() {
    return reg64Buffer;
  }
  inline reg32* getRegs32() {
    return reg32Buffer;
  }
  inline reg64* getReg64(int regno) {
    return &reg64Buffer[regno];
  }
  inline reg32* getReg32(int regno) {
    return &reg32Buffer[regno];
  }
  inline void setReg64(int regno, reg64 val) {
    reg64Buffer[regno] = val;
  }
  inline void setReg32(int regno, reg32 val) {
    reg32Buffer[regno] = val;
  }

  // manipulate stored registers
  // thread-safe as only called by its own CPU
  inline void decrementRip() {
    reg64Buffer[registers::RIP] -= 1;
  }
  inline void incrementRip() {
    unsigned char opCode = *(unsigned char *) reg64Buffer[registers::RIP];
    if (opCode == 0xcc) {
      reg64Buffer[registers::RIP] += 1;
      DBG::outln(DBG::GDBDebug, "rip incremented");
    }
  }
} __attribute__((aligned(4096)));   // required to compile


extern "C" GdbCpu* getCurrentGdbCpu();
extern "C" uint64_t* getCurrentGdbStack(GdbCpu*);

#endif // GdbCpu_h_
