#ifndef _IThread_h_
#define _IThread_h_ 1

class IEvent;
class Thread;
class Interrupt;

extern void ithreadLoop(ptr_t arg);

class IThread {
  friend Interrupt;
  friend void ithreadLoop(ptr_t);
  Thread* thread;
  IEvent* event;
  void (*threadLoop)(ptr_t);
  bool need;

public:
  IThread(Thread* t, IEvent* e) : thread(t), event(e), threadLoop(ithreadLoop), need(false) {}
  void executeHandlers();
  void schedule();
};

#endif /* _IThread_h_ */
