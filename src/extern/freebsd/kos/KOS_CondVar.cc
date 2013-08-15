#include "kos/CondVar.h"

// KOS
#include <cstdint>
#include "ipc/CV.h"
#include "ipc/ReadWriteLock.h"

void KOS_CV_Init(void** cvp) {
  *cvp = new CV();
}

// mutex can be sleep mutex, sx, or rw lock
void KOS_CV_Wait(void* cv, void* mutex, int lockType, int unlock) {
  CV* cond = static_cast<CV*>(cv);
  switch (lockType) {
    case 0: {
      Mutex* mtx = static_cast<Mutex*>(mutex);
      cond->wait(*mtx, unlock ? true : false);
    } break;
    case 1: {
      RwMutex* mtx = static_cast<RwMutex*>(mutex);
      cond->wait(*mtx, unlock ? true : false);
    } break;
    default: ABORT1("invalid lock type");
  }
}

void KOS_CV_Signal(void* cv) {
  CV* cond = static_cast<CV*>(cv);
  cond->signal();  // FIXME
}

void KOS_CV_BroadCast(void* cv) {
  CV* cond = static_cast<CV*>(cv);
  cond->broadcast(); // FIXME
}

void KOS_CV_Destroy(void** cv) {
  CV* cond = static_cast<CV*>(*cv);
  delete cond;
  *cv = 0;
}
