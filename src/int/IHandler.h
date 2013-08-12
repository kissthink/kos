#ifndef _IHandler_h_
#define _IHandler_h_ 1

#include "kern/Kernel.h"
#include "ipc/SleepQueue.h"

typedef int filterFunc(ptr_t);
typedef void handlerFunc(ptr_t);

class IEvent;
class IThread;
class Interrupt;

class IHandler {
  friend Interrupt;
  friend IThread;
  filterFunc* filter;     // runs in interrupt context
  handlerFunc* handler;   // runs in ithread context
  ptr_t arg;
  kstring name;
  bool exclusive;
  bool need;
  bool pendingRemoval;
  bool run;
  IEvent* event;

public:
  IHandler(filterFunc filter, handlerFunc handler, ptr_t arg, const char* name, bool exclusive)
  : filter(filter), handler(handler), arg(arg), name(name)
  , exclusive(exclusive), need(false), pendingRemoval(false), run(false), event(nullptr) {}

  void setEvent(IEvent*);
  void remove() {
    pendingRemoval = true;
  }
  bool shouldRemove() {
    return pendingRemoval;
  }
  bool isExclusive() {
    return exclusive;
  }
  bool hasHandler() {
    return handler != nullptr;
  }
  void schedule() {
    need = true;
  }
  bool running() {
    return run;
  }
};

#endif /* _IHandler_h_ */
