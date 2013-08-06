#include <cstdint>
#include "ipc/ReadWriteLock.h"
#include "ipc/BlockingSync.h"
#include "ipc/CondVar.h"

void RwMutex::readAcquire() {
  mtx.acquire();
  while (writer) {
    cond.wait(&mtx);
  }
  readers += 1;
  mtx.release();
}

void RwMutex::readRelease() {
  mtx.acquire();
  KASSERT1(!writer, "releasing readlock while holding a writelock");
  readers -= 1;
  if (readers == 0) {
    cond.broadcast();
  }
  mtx.release();
}

void RwMutex::writeAcquire() {
  mtx.acquire();
  while (readers || writer) {
    cond.wait(&mtx);
  }
  writer = true;
  mtx.release();
}

void RwMutex::writeRelease() {
  mtx.acquire();
  KASSERT1(!readers, "releasing writelock while holding a readlock");
  writer = false;
  cond.broadcast();
  mtx.release();
}

bool RwMutex::tryReadAcquire() {
  if (mtx.tryAcquire()) {
    if (!writer) {
      readers += 1;
      mtx.release();
      return true;
    }
    mtx.release();
  }
  return false;
}

bool RwMutex::tryWriteAcquire() {
  if (mtx.tryAcquire()) {
    if (!readers && !writer) {
      writer = true;
      mtx.release();
      return true;
    }
    mtx.release();
  }
  return false;
}

bool RwMutex::tryUpgrade() {
  if (mtx.tryAcquire()) {
    if (readers == 1) {
      readers = 0;
      writer = true;
      mtx.release();
      return true;
    }
    mtx.release();
  }
  return false;
}

void RwMutex::downGrade() {
  mtx.acquire();
  KASSERT1(writer, "cannot downgrade readlock");
  writer = false;
  readers = 1;
  cond.broadcast();
  mtx.release();
}
