#ifndef _TimerEvent_h_
#define _TimerEvent_h_ 1

#include "mach/Machine.h"

enum class TimerEventMode {
  OneShot,
  Periodic
};

// abstract class representing timer event
class TimerEvent {
  mword timeoutPeriod;
  mword nextTimeout;
  TimerEventMode mode;

public:
  TimerEvent(mword timeout, bool oneshot=false) : timeoutPeriod(timeout), nextTimeout(Machine::now() + timeout), mode(oneshot ? TimerEventMode::OneShot : TimerEventMode::Periodic) {}
  virtual ~TimerEvent() {}
  virtual bool run(mword currentTick) = 0;
  bool timedout() const {
    return Machine::now() >= nextTimeout;
  }
  void reset() {
    nextTimeout = Machine::now() + timeoutPeriod;
  }
  void setMode(TimerEventMode m) {
    mode = m;
  }
  bool isOneShot() const {
    return mode == TimerEventMode::OneShot;
  }
};

#endif /* _TimerEvent_h_ */
