#ifndef _IEvent_h_
#define _IEvent_h_ 1

#include "util/basics.h"
#include "extern/stl/mod_set"
#include "ipc/SpinLock.h"

class IHandler;
class ISource;
class IThread;

class IEvent {
  friend IThread;
  InPlaceSet<IHandler*,0,less<IHandler*>> handlers;
  IThread* ithread;
  ISource* source;
  bool soft;
  bool creatingThread;
  SpinLock lk;

public:
  IEvent(IThread* t, ISource* source) : ithread(t), source(source)
  , soft(false), creatingThread(false) {}

  void addHandler(IHandler* h);
  void removeHandler(IHandler* h);
  void createIThread(const char* name);
  bool isSoftInterrupt() {
    return soft;
  }
  void setSoft() {
    soft = true;
  }
  int getVector();
  int getIRQ();
};

#endif /* _IEvent_h_ */
