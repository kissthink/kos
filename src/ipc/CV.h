#ifndef _CV_h_
#define _CV_h_ 1

#include "ipc/BlockingSync.h"

class RwMutex;
class CV {  // reference: http://research.microsoft.com/pubs/64242/implementingcvs.pdf
  Semaphore s,x,h;
  mword waiters;
public:
  CV() : s(0), x(1), h(0), waiters(0) {}
  void wait(Mutex& m, bool unlock = false) {
    KASSERT1(m.isLocked(), "cannot wait on an unlocked mutex");
    x.P();
    waiters += 1;
    x.V();
    m.release();
    s.P();
    h.V();
    if (!unlock) m.acquire();
  }
  void wait(RwMutex& m, bool unlock = false);
  void signal() {
    x.P();
    if (waiters) {
      waiters -= 1;
      s.V();
      h.P();
    }
    x.V();
  }
  void broadcast() {
    x.P();
    for (mword i = 0; i < waiters; i++) s.V();
    while (waiters) { waiters -= 1; h.P(); }
    x.V();
  }
};
#endif /* _CV_h_ */
