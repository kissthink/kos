#ifndef _Bus_h_
#define _Bus_h_ 1

#include "mach/Device.h"

class Bus : public Device {
  Bus(const Bus&) = delete;
  Bus& operator=(const Bus&) = delete;

public:
  Bus(const char* name) : Device(name, Device::Bus) {}
};

#endif /* _Bus_h_ */
