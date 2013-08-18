#include "mach/InterruptManager.h"
#include "mach/InterruptHandler.h"
#include "kern/Debug.h"
#include "util/basics.h"

SpinLock InterruptManager::lk;
InterruptHandler* InterruptManager::genHandlers[256];
InterruptHandlerWithError* InterruptManager::errHandlers[256];

void InterruptManager::init() {
  for (int i = 0; i < 256; i++) {
    genHandlers[i] = nullptr;
    errHandlers[i] = nullptr;
  }
}

void InterruptManager::handleInterrupt(mword vector) {
  ScopedLock<> lo(lk);
  InterruptHandler* handler = genHandlers[vector];
  if (!handler) {
    ABORTN("interrupt handler not registered for vector:", vector);
    return;
  }
  handler->handle(vector);
}

void InterruptManager::handleInterrupt(mword vector, mword errcode) {
  ScopedLock<> lo(lk);
  InterruptHandlerWithError* handler = errHandlers[vector];
  if (!handler) {
    ABORTN("interrupt handler not registered for vector:", vector);
    return;
  }
  handler->handle(vector, errcode);
}

bool InterruptManager::registerHandler(mword vector, InterruptHandler* h) {
  ScopedLock<> lo(lk);
  if unlikely(h == nullptr) {
    ABORT1("interrupt handler is null");
  }
  if unlikely(vector >= 0x100) return false;
  if unlikely(h && genHandlers[vector]) return false;
  genHandlers[vector] = h;
  return true;
}

bool InterruptManager::registerHandler(mword vector, InterruptHandlerWithError* h) {
  ScopedLock<> lo(lk);
  if unlikely(h == nullptr) {
    ABORT1("interrupt handler is null");
  }
  if unlikely(vector >= 0x100) return false;
  if unlikely(h && errHandlers[vector]) return false;
  errHandlers[vector] = h;
  return true;
}

bool InterruptManager::unregisterHandler(mword vector) {
  ScopedLock<> lo(lk);
  if unlikely(vector >= 0x100) return false;
  if unlikely(!genHandlers[vector] && !errHandlers[vector]) return false;
  genHandlers[vector] = nullptr;
  errHandlers[vector] = nullptr;
  return true;
}
