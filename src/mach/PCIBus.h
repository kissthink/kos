#ifndef _PCIBus_h_
#define _PCIBus_h_ 1

#include "mach/Bus.h"

class PCIBus : public Bus {
  PCIBus(const PCIBus&) = delete;
  PCIBus& operator=(const PCIBus&) = delete;

  uint8_t busNum;
public:
  PCIBus(uint8_t busNum) : Bus("PCI-Bus"), busNum(busNum) {}
  uint8_t getBusNum() const {
    return busNum;
  }
};

#endif /* _PCIBus_h_ */
