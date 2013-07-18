#include "gdb/Gdb.h"

GdbCpu* Gdb::gdbCPUs = nullptr;
int Gdb::numCpu = 0;
int Gdb::numInitialized = 0;
int Gdb::enumIdx = 0;
NonBlockSemaphore* Gdb::sem = nullptr;
SpinLock Gdb::mutex;
