#ifndef _PCIDevice_h_
#define _PCIDevice_h_ 1

#include "mach/Device.h"

class PCIDevice : public Device {
  PCIDevice(const PCIDevice&) = delete;
  PCIDevice& operator=(const PCIDevice&) = delete;

  uint8_t busNum;
  uint8_t deviceNum;
  uint8_t funcNum;

  inline uint32_t readConfigWord(uint16_t reg) const {
    return PCI::readConfigWord( busNum, deviceNum, funcNum, reg, 32 );
  }

public:
  PCIDevice(uint8_t bus, uint8_t dev, uint8_t func, const char* name)
      : Device(name), busNum(bus), deviceNum(dev), funcNum(func) {}
  inline uint16_t getVendorID() const {
    return PCI::VendorID.get( readConfigWord(0) );
  }
  inline uint16_t getDeviceID() const {
    return PCI::DeviceID.get( readConfigWord(0) );
  }
  inline uint16_t getCommand() const {
    return PCI::Command.get( readConfigWord(1) );
  }
  inline uint16_t getStatus() const {
    return PCI::Status.get( readConfigWord(1) );
  }
  inline uint8_t getRevision() const {
    return PCI::Revision.get( readConfigWord(2) );
  }
  inline uint8_t getProgIF() const {
    return PCI::ProgIF.get( readConfigWord(2) );
  }
  inline uint8_t getSubClass() const {
    return PCI::SubClass.get( readConfigWord(2) );
  }
  inline uint8_t getClassCode() const {
    return PCI::ClassCode.get( readConfigWord(2) );
  }
  inline uint8_t getCacheLineSize() const {
    return PCI::CacheLineSize.get( readConfigWord(3) );
  }
  inline uint8_t getLatencyTimer() const {
    return PCI::LatencyTimer.get( readConfigWord(3) );
  }
  inline uint8_t getHeaderType() const {
    return PCI::HeaderType.get( readConfigWord(3) );
  }
  inline uint8_t getBIST() const {
    return PCI::BIST.get( readConfigWord(3) );
  }
};

#endif /* _PCIDevice_h_ */
