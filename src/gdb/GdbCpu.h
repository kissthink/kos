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
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15, RIP,
    EFLAGS = 0, CS, SS, DS, ES, FS, GS
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
const int bufferSize = (1<<20);

class GdbCpu {
  // *do NOT change orders for these 4 variables*
  reg64 reg64Buffer[numRegs64];       // gdb_asm_functions.S requires this here
  reg32 reg32Buffer[numRegs32];
  uint64_t stack[bufferSize];
  int64_t gdbErrorCode;

  char cpuId[20];                     // in format (Core %d) [State]
  string cpuInfo;                // used by GDB RSP (remote serial protocol)
  int cpuIndex;

  cpuState::cpuStateEnum state;
  SpinLock mutex;

  bool ripDecremented;                // rip is decremented for int 3 with 's'


  // CPU string used by 'info thread'
  void setCpuIdStr() {
    ScopedLock<> so(mutex);
    _setCpuIdStr();
  }
  // unlocked internal implementation
  void _setCpuIdStr() {
    memset(cpuId, 0, sizeof(char) * 20);
    snprintf(cpuId, 20, "CPU %d [%s]", cpuIndex+1, cpuState::cpuStateStr[state]);
  }

public:
  GdbCpu(): gdbErrorCode(0), state(cpuState::UNKNOWN), ripDecremented(false) {
    memset(cpuId, 0, sizeof(char) * 20);
    memset(stack, 0, sizeof(uint64_t) * bufferSize); 
    memset(reg64Buffer, 0, numRegs64 * sizeof(reg64));
    memset(reg32Buffer, 0, numRegs32 * sizeof(reg32));
  }

  // sets string m1, m2, ... (used for info thread)
  void setCpuId(int cpuIndex) {
    ScopedLock<> so(mutex);
    this->cpuIndex = cpuIndex;
    _setCpuIdStr();
    char buf2[10];
    snprintf(buf2, 10, "m%d", cpuIndex+1);
    cpuInfo = buf2;
  }

  // sets cpu state enum and string
  void setCpuState(cpuState::cpuStateEnum newState) {
    ScopedLock<> so(mutex);
    state = newState;
    _setCpuIdStr();
  }

  // returns cpu state enum
  cpuState::cpuStateEnum getCpuState() {
    ScopedLock<> so(mutex);
    return state;
  }

  // returns cpu name
  inline const char* getId() {
    ScopedLock<> so(mutex);
    return cpuId;
  }

  // returns cpu index (starts from 0)
  int getCpuIndex() {
    ScopedLock<> so(mutex);
    return cpuIndex;
  }
  /**
   * Returns CPU's info, used by GDB.
   * (e.g. m1,m2,m3,m4)
   */
  inline string getCpuInfo() {
    ScopedLock<> so(mutex);
    return cpuInfo;
  }

  uint64_t* stackPtr() {           // starting address of stack setup by GDB stub
    ScopedLock<> so(mutex);
    return stack + bufferSize;
  }

  // access registers stored on reg64/32 buffers
  reg64* getRegs64() {
    ScopedLock<> so(mutex);
    return reg64Buffer;
  }
  reg32* getRegs32() {
    ScopedLock<> so(mutex);
    return reg32Buffer;
  }
  reg64* getReg64(int regno) {
    ScopedLock<> so(mutex);
    return &reg64Buffer[regno];
  }
  reg32* getReg32(int regno) {
    ScopedLock<> so(mutex);
    return &reg32Buffer[regno];
  }
  void setReg64(int regno, reg64 val) {
    ScopedLock<> so(mutex);
    reg64Buffer[regno] = val;
  }
  void setReg32(int regno, reg32 val) {
    ScopedLock<> so(mutex);
    reg32Buffer[regno] = val;
  }

  // rip manipulation
  void decrementRip() {
    ScopedLock<> so(mutex);
    if (!ripDecremented) {
      reg64Buffer[registers::RIP] -= 1;
      ripDecremented = true;
    }
  }
  void incrementRip() {
    ScopedLock<> so(mutex);
    if (ripDecremented) {
      reg64Buffer[registers::RIP] += 1;
      ripDecremented = false;
      DBG::outln(DBG::GDBDebug, "rip incremented");
    }
  }
  void resetRip() {
    ScopedLock<> so(mutex);
    ripDecremented = false;
  }

} __attribute__((aligned(4096)));   // required to compile


extern "C" GdbCpu* getCurrentGdbCpu();
extern "C" uint64_t* getCurrentGdbStack(GdbCpu*);

#endif // GdbCpu_h_
