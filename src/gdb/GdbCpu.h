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

class GdbCpuState
{
    // do NOT change orders for these 4 variables
    reg64 reg64Buffer[numRegs64];       // gdb_asm_functions.S requires this here
    reg32 reg32Buffer[numRegs32];
    uint64_t stack[bufferSize/sizeof(uint64_t)];
    int64_t gdbErrorCode;   // 164

    char cpuId[20];         // in format (Core %d) [State]
    std::string cpuInfo;    // used by GDB RSP (remote serial protocol)
    int cpuIndex;

    cpuState::cpuStateEnum state;
    SpinLock mutex;

    /**
     * Updates cpuId string used by 'info thread'.
     */
    void setCpuIdStr();

    /**
     * Updates cpuId string used by 'info thread'.
     */
    void _setCpuIdStr();

public:
    GdbCpuState();

    /**
     * Assigns cpu index and initializes cpuId string.
     */
    void setCpuId(int cpuIndex);

    /**
     * Updates cpu state.
     */
    void setCpuState(cpuState::cpuStateEnum newState);

    /**
     * Get current cpu state.
     */
    cpuState::cpuStateEnum getCpuState();

    /**
     * Returns current CPU's id string.
     */
    inline const char* getId() {
        ScopedLock<> so(mutex);
        return cpuId;
    }

    /**
     * Returns current CPu's index.
     */
    int getCpuIndex() {
        ScopedLock<> so(mutex);
        return cpuIndex;
    }

    /**
     * Returns CPU's info, used by GDB.
     * (e.g. m1,m2,m3,m4)
     */
    inline std::string getCpuInfo() {
        ScopedLock<> so(mutex);
        return cpuInfo;
    }

    /**
     * Returns starting address of stack,
     * used for storing registers.
     */
    uint64_t* stackPtr();

    /**
     * Save registers and go to the exception handler.
     */
    void saveRegisters();

    /**
     * Returns from the exception handler.
     */
    void restoreRegisters();

    /**
     * Returns the address to a buffer storing 64-bit registers.
     */
    reg64* getRegs64();

    /**
     * Returns the address to a buffer storing 32-bit registers.
     */
    reg32* getRegs32();

    /**
     * Returns the address to a 64-bit register.
     */
    reg64* getReg64(int regno);

    /**
     * Returns the address to a 32-bit register.
     */
    reg32* getReg32(int regno);

    /**
     * Updates a 64-bit register value.
     */
    void setReg64(int regno, reg64 val);

    /**
     * Updates a 32-bit register value.
     */
    void setReg32(int regno, reg32 val);


} __attribute__((aligned(4096)));

};  // namespace gdb

#endif // GdbCpu_h_
