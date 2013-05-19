#ifndef GdbCpu_h_
#define GdbCpu_h_ 1

#include <string>
#include "util/SpinLock.h"

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
}

const int numRegs64 = 17;
const int numRegs32 = 7;
static const int bufferSize = 10000;

class GdbCpuState {
  // *do NOT change orders for these 4 variables*
  reg64 reg64Buffer[numRegs64];       // gdb_asm_functions.S requires this here
  reg32 reg32Buffer[numRegs32];
  uint64_t stack[bufferSize/sizeof(uint64_t)];
  int64_t gdbErrorCode;

  char cpuId[20];         // in format (Core %d) [State]
  std::string cpuInfo;    // used by GDB RSP (remote serial protocol)
  int cpuIndex;

  cpuState::cpuStateEnum state;
  SpinLock mutex;

  void setCpuIdStr();     // CPU string used by 'info thread'
  void _setCpuIdStr();

public:
  GdbCpuState();

  void setCpuId(int cpuIndex);
  void setCpuState(cpuState::cpuStateEnum newState);

  cpuState::cpuStateEnum getCpuState();
  inline const char* getId() {
    ScopedLockISR<> so(mutex);
    return cpuId;
  }
  int getCpuIndex() {
    ScopedLockISR<> so(mutex);
    return cpuIndex;
  }
  /**
   * Returns CPU's info, used by GDB.
   * (e.g. m1,m2,m3,m4)
   */
  inline std::string getCpuInfo() {
    ScopedLockISR<> so(mutex);
    return cpuInfo;
  }

  uint64_t* stackPtr();           // starting address of stack setup by GDB stub

  void saveRegisters();           // save registers and go to exception handler
  void restoreRegisters();        // restore registers and return to KOS

  // access registers stored on reg64/32 buffers
  reg64* getRegs64();
  reg32* getRegs32();
  reg64* getReg64(int regno);
  reg32* getReg32(int regno);
  void setReg64(int regno, reg64 val);
  void setReg32(int regno, reg32 val);


} __attribute__((aligned(4096)));   // required to compile

};  // namespace gdb

#endif // GdbCpu_h_
