#include "kos/Callout.h"

// KOS
#include <cstdint>
#include "ipc/SpinLock.h"
#include "kern/Kernel.h"
#include <map>

static SpinLock calloutLock;
static map<ptr_t,ptr_t,less<ptr_t>,KernelAllocator<pair<ptr_t,ptr_t>>> calloutMap;

ptr_t KOS_GetPerThreadCallout(ptr_t td) {
  ScopedLock<> lo(calloutLock);
  if (!calloutMap.count(td)) return nullptr;
  return calloutMap[td];
}

void KOS_SetPerThreadCallout(ptr_t td, ptr_t co) {
  ScopedLock<> lo(calloutLock);
  KASSERTN(!calloutMap.count(td), "callout for thread:", FmtHex(td), " exists");
  calloutMap[td] = co;
}
