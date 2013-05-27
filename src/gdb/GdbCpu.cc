#include "gdb/GdbCpu.h"

// needed this implementation in .cc for gdb_asm_functions.S
uint64_t* GdbCpuState::stackPtr() {
  ScopedLockISR<> so(mutex);
  return stack + bufferSize;
}
