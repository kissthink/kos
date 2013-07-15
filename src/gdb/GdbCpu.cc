#include "gdb/GdbCpu.h"
#include "mach/Processor.h"

GdbCpu* getCurrentGdbCpu() {
  return Processor::getGdbCpu();
}

uint64_t* getCurrentGdbStack(GdbCpu* gs) {
  return gs->stackPtr();
}