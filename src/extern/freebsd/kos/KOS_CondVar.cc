#include "kos/CondVar.h"

// KOS
#include <cstdint>
#include "ipc/CondVar.h"
#include "ipc/ReadWriteLock.h"

void KOS_CV_Init(void** cvp) {
  *cvp = new ConditionVar();
}

// mutex can be sleep mutex, sx, or rw lock
void KOS_CV_Wait(void* cv, void* mutex, int lockType, int unlock) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  switch (lockType) {
    case 0: {
      Mutex* mtx = static_cast<Mutex*>(mutex);
      cond->wait(mtx, unlock ? true : false);
    } break;
    case 1: {
      RwMutex* mtx = static_cast<RwMutex*>(mutex);
      cond->wait(mtx, unlock ? true : false);
    } break;
    default: ABORT1("invalid lock type");
  }
}

void KOS_CV_Signal(void* cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  cond->signal();  // FIXME
}

void KOS_CV_BroadCast(void* cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  cond->broadcast(); // FIXME
}

void KOS_CV_Destroy(void** cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(*cv);
  delete cond;
  *cv = 0;
}
