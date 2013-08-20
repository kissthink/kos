#include "kern/DynamicTimer.h"
#include "kern/Debug.h"
using namespace std;

SpinLock DynamicTimer::lk;
list<TimerEvent*,KernelAllocator<TimerEvent*>> DynamicTimer::events;
typedef list<TimerEvent*,KernelAllocator<TimerEvent*>> TimerList;

void DynamicTimer::registerEvent(TimerEvent* event) {
  ScopedLock<> lo(lk);
  events.push_back(event);
}

void DynamicTimer::unregisterEvent(TimerEvent* event) {
  ScopedLock<> lo(lk);
  TimerList::iterator it = events.begin();
  while (it != events.end()) {
    if ((*it) == event) {
      events.erase(it);
      break;
    }
    ++it;
  }
}

void DynamicTimer::run(mword currTick) {
  ScopedLock<> lo(lk);
  TimerList::iterator it = events.begin();
  while (it != events.end()) {
    bool ran = (*it)->run(currTick);
    if (ran) {
      if ((*it)->isOneShot()) {
        if ((*it)->timedout()) {
          events.erase(it++);
          DBG::outln(DBG::Basic, "event removed");
          continue;
        } // else oneshot mode but resetted to new timeout
      } else {
        (*it)->reset();
      }
    }
    ++it;
  }
}
