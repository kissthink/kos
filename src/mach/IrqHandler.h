#ifndef _IrqHandler_h_
#define _IrqHandler_h_ 1

class IrqHandler {
protected:
  inline virtual ~IrqHandler() {}
public:
  virtual bool irq(uint8_t vec) = 0;
};

#endif /* _IrqHandler_h_ */
