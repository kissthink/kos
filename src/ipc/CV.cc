#include <cstdint>
#include "ipc/CV.h"
#include "ipc/ReadWriteLock.h"

void CV::wait(RwMutex& m, bool unlock) {
  KASSERT1(m.isLocked(), "cannot wait on an unlocked rw mutex");
  x.P();
  waiters += 1;
  x.V();
  bool writeLock = m.isExclusive();
  if (writeLock) {
    m.writeRelease();
  } else {
    m.readRelease();
  }
  s.P();
  h.V();
  if (!unlock) {
    if (writeLock) {
      m.writeAcquire();
    } else {
      m.readAcquire();
    }
  }
}