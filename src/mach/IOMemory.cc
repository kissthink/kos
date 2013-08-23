#include "mach/IOMemory.h"
#include "mach/Memory.h"
#include "kern/Kernel.h"

void IOMemory::releaseMe() {
  bool res = false;
  if ( memSize >= 0x200000 ) {
    kernelSpace.unmapPages<2>( baseVAddr, memSize );
  } else {
    kernelSpace.unmapPages<1>( baseVAddr, memSize );
  }
  if (!res)
    DBG::outln(DBG::VM, "IOMemory::releaseMe() - unmapping address: ", FmtHex(baseVAddr), " failed");
}
