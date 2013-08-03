#include "kos/CondVar.h"

// KOS
#include <cstdint>
#include "ipc/CondVar.h"

extern "C" void KOS_CV_Init(void** cvp) {
  *cvp = new ConditionVar();
}

// mutex can be sleep mutex, sx, or rw lock
extern "C" void KOS_CV_Wait(void* cv, void* mutex, int lockType, int unlock) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  switch (lockType) {
    case 0: {
      Mutex* mtx = static_cast<Mutex*>(mutex);
      cond->wait(mtx, unlock ? false : true);
    } break;
    case 1: {
      RwMutex* mtx = static_cast<RwMutex*>(mutex);
      cond->wait(mtx, unlock ? false : true);
    } break;
    default: ABORT1("invalid lock type");
  }
}

extern "C" void KOS_CV_Signal(void* cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  cond->signal();
}

extern "C" void KOS_CV_BroadCast(void* cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  cond->broadcast();
}

extern "C" void KOS_CV_Destroy(void** cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(*cv);
  delete cond;
  *cv = 0;
}
