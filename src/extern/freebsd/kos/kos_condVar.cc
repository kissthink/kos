#include <cstdint>
#include "ipc/CondVar.h"

void kos_cv_init(void** cvp) {
  *cvp = new ConditionVar();
}

// mutex can be sleep mutex, sx, or rw lock
void kos_cv_wait(void* cv, void* mutex, int lockType, int unlock) {
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

void kos_cv_signal(void* cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  cond->signal();
}

void kos_cv_broadcast(void* cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(cv);
  cond->broadcast();
}

void kos_cv_destroy(void** cv) {
  ConditionVar* cond = static_cast<ConditionVar*>(*cv);
  delete cond;
  *cv = 0;
}
