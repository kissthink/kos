#include "kos/Timer.h"

// KOS
#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include "mach/Machine.h"
#include "util/Output.h"
#include <map>
using namespace std;

int KOS_Ticks() {
  return Machine::now();  // PIT tick
}

static SpinLock volTickLock;
static map<ptr_t,int,less<ptr_t>,KernelAllocator<pair<ptr_t,int>>> volTickMap;

void KOS_UpdateVolTick(ptr_t td) {
  ScopedLock<> lo(volTickLock);
  volTickMap[td] = KOS_Ticks();
}

int KOS_GetVolTick(ptr_t td) {
  ScopedLock<> lo(volTickLock);
  KASSERT1(volTickMap.count(td), "unknown thread");
  return volTickMap[td];
}
