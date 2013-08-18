#ifndef _Controller_h_
#define _Controller_h_ 1

#include "mach/Device.h"

class Controller : public Device {
public:
  Controller() {}
  Controller(Device* p) : Device(p) {}
  virtual ~Controller() {}
  virtual Type getType() {
    return Device::Controller;
  }
  virtual void getName(String& str) {
    str = "Generic controller";
  }
  virtual void dump(String& str) {
    str = "Generic controller";
  }
};

#endif /* _Controller_h_ */
