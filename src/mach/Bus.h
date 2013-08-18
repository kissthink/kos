#ifndef __Bus_h_
#define __Bus_h_ 1

#include "mach/Device.h"

class Bus : public Device {
  Bus(const Bus&) = delete;
  Bus& operator=(const Bus&) = delete;

  const char* name;

public:
  Bus(const char* name) : name(name) {}
  ~Bus() {}
  Type getType() {
    return Device::Bus;
  }
  void getName(kstring& str) {
    str = name;
  }
  void dump(kstring& str) {
    str = name;
  }
};

#endif /* _Bus_h_ */
