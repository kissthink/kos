#ifndef _ISource_h_
#define _ISource_h_ 1

class IEvent;

class ISource {
  IEvent* event;
  int vector;
  int irq;

public:
  ISource(IEvent* e, int v, int i) : event(e), vector(v), irq(i) {}
  int getVector() {
    return vector;
  }
  int getIRQ() {
    return irq;
  }
};

#endif /* _ISource_h_ */
