#ifndef _InterruptHandler_h_
#define _InterruptHandler_h_ 1

class InterruptHandler {
protected:
  virtual ~InterruptHandler() {}
public:
  virtual void handle(mword vector) = 0;
};

class InterruptHandlerWithError {
protected:
  virtual ~InterruptHandlerWithError() {}
public:
  virtual void handle(mword vector, mword errcode) = 0;
};

#endif /* _InterruptHandler_h_ */
