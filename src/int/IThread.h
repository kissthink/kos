#ifndef _IThread_h_
#define _IThread_h_ 1

class IEvent;
class Thread;

class IThread {
  Thread* thread;
  IEvent* event;

public:
  IThread(Thread* t, IEvent* e) : thread(t), event(e) {}
  void executeHandlers();
};

#endif /* _IThread_h_ */
