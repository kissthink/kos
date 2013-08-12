#ifndef _IHandler_h_
#define _IHandler_h_ 1

#include "kern/Kernel.h"

typedef int filterFunc(ptr_t);
typedef void handlerFunc(ptr_t);

class IEvent;
class IThread;

class IHandler {
  friend IThread;
  filterFunc* filter;     // runs in interrupt context
  handlerFunc* handler;   // runs in ithread context
  ptr_t arg;
  kstring name;
  bool exclusive;
  bool need;
  bool pendingRemoval;
  IEvent* event;

public:
  IHandler(filterFunc filter, handlerFunc handler, ptr_t arg, const char* name, bool exclusive)
  : filter(filter), handler(handler), arg(arg), name(name)
  , exclusive(exclusive), need(false), pendingRemoval(false), event(nullptr) {}

  void setEvent(IEvent*);
  void remove() {
    pendingRemoval = true;
  }
  bool shouldRemove() {
    return pendingRemoval;
  }
};

#endif /* _IHandler_h_ */
