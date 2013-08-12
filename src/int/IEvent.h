#ifndef _IEvent_h_
#define _IEvent_h_ 1

#include "util/basics.h"
#include "extern/stl/mod_set"
#include "ipc/SpinLock.h"
#include "int/Interrupt.h"
#include "kern/Kernel.h"

class IHandler;
class ISource;
class IThread;
class Interrupt;

class IEvent {
  friend IThread;
  friend Interrupt;
  using HandlerSet = set<IHandler*,less<IHandler*>,KernelAllocator<IHandler*>>;
  HandlerSet handlers;
  IThread* ithread;
  ISource* source;
  preIThreadFunc* preIThread;
  postIThreadFunc* postIThread;
  postFilterFunc* postFilter;
  bool soft;
  SpinLock lk;

  void createIThread(const char* name);
public:
  IEvent(ISource* source, preIThreadFunc f1, postIThreadFunc f2, postFilterFunc f3, bool soft)
  : ithread(nullptr), source(source), preIThread(f1), postIThread(f2), postFilter(f3), soft(soft) {}
  bool addHandler(IHandler* h);
  void removeHandler(IHandler* h);
  bool isSoftInterrupt() {
    return soft;
  }
  int getVector();
  int getIRQ();
  bool hasHandlers() {
    return !handlers.empty();
  }
};

#endif /* _IEvent_h_ */
