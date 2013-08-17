#include "kos/Interrupt.h"

// KOS
#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include "util/Output.h"
#include <map>
using namespace std;

struct InterruptData {
  bool iwait;
  InterruptData() : iwait(false) {}
};

static SpinLock intLock;
static map<ptr_t,InterruptData*,less<ptr_t>,KernelAllocator<pair<ptr_t,InterruptData*>>> intMap;

void KOS_IntrSetIWAIT(ptr_t td) {
  ScopedLock<> lo(intLock);
  if (intMap.count(td) == 0) {
    intMap[td] = new InterruptData;
  }
  intMap[td]->iwait = true;
}

void KOS_IntrResetIWAIT(ptr_t td) {
  ScopedLock<> lo(intLock);
  KASSERTN(intMap.count(td), "unknown thread:", FmtHex(td));
  intMap[td]->iwait = false;
}

int KOS_IntrAwaitingIntr(ptr_t td) {
  ScopedLock<> lo(intLock);
  KASSERTN(intMap.count(td), "unknown thread:", FmtHex(td));
  return intMap[td]->iwait ? 1 : 0;
}
