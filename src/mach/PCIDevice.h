#ifndef _PCIDevice_h_
#define _PCIDevice_h_ 1

#include "mach/Device.h"
#include "mach/PCI.h"

class PCIDevice : public Device {
  PCIDevice(const PCIDevice&) = delete;
  PCIDevice& operator=(const PCIDevice&) = delete;

  uint8_t busNum;
  uint8_t deviceNum;
  uint8_t funcNum;

  inline uint32_t readConfigWord(uint16_t reg) const {
    return PCI::readConfigWord( busNum, deviceNum, funcNum, reg, 32 );
  }
  inline void writeConfigWord(uint16_t reg, uint32_t val) const {
    PCI::writeConfigWord( busNum, deviceNum, funcNum, reg, 32, val);
  }

public:
  PCIDevice(uint8_t bus, uint8_t dev, uint8_t func, const char* name)
      : Device(name, Device::PCI), busNum(bus), deviceNum(dev), funcNum(func) {}
  inline uint8_t getBus() const {
    return busNum;
  }
  inline uint8_t getDevice() const {
    return deviceNum;
  }
  inline uint8_t getFunction() const {
    return funcNum;
  }
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
  uint8_t getInterruptNumber() const {
    switch (PCI::HeaderType.get(readConfigWord(3)) & 0x7f) {
      case 0x0: // standard
        return PCI::PCIGeneral::InterruptLine.get(readConfigWord(15));
      case 0x1: // PCI-PCI bridge
        return PCI::PCItoPCIBridge::InterruptLine.get(readConfigWord(15));
      case 0x2: // CardBus bridge
        return PCI::PCItoCardBusBridge::InterruptLine.get(readConfigWord(15));
      default:
        ABORT1("unknown header type");
    }
    return 0;
  }
  uint8_t getInterruptPin() const {
    switch (PCI::HeaderType.get(readConfigWord(3)) & 0x7f) {
      case 0x0: // standard
        return PCI::PCIGeneral::InterruptPin.get(readConfigWord(15));
      case 0x1: // PCI-PCI bridge
        return PCI::PCItoPCIBridge::InterruptPin.get(readConfigWord(15));
      case 0x2: // CardBus bridge
        return PCI::PCItoCardBusBridge::InterruptPin.get(readConfigWord(15));
      default:
        ABORT1("unknown header type");
        return 0;
    }
  }
  uint32_t getBAR(uint8_t idx) const {
    switch (PCI::HeaderType.get(readConfigWord(3)) & 0x7f) {
      case 0x0: // standard
        KASSERT1( idx < 6, idx );
        return readConfigWord( idx + 4 );
      case 0x1: // PCI-PCI bridge
        KASSERT1( idx < 2, idx );
        return readConfigWord( idx + 4 );
      default:
        ABORT1("BAR doesn't exist");
        return 0;
    }
  }
  uint32_t getBARSize(uint8_t idx) const {
    switch (PCI::HeaderType.get(readConfigWord(3)) & 0x7f) {
      case 0x0: // standard
        KASSERT1( idx < 6, idx ); break;
      case 0x1: // PCI-PCI bridge
        KASSERT1( idx < 2, idx ); break;
      default:
        ABORT1("BAR doesn't exist");
    }
    uint32_t bar = readConfigWord( idx + 4 );   // read bar to save
    writeConfigWord( idx + 4, 0xffffffff );     // write all 1's
    uint32_t mask = readConfigWord( idx + 4 );  // read mask
    writeConfigWord( idx + 4, bar );            // restore bar
    uint32_t size = ~(mask & 0xfffffff0) + 1;
    if (bar & 0x1) {
      size &= 0xffff;                           // I/O space only 64kb
    }
    return size;
  }
};

#endif /* _PCIDevice_h_ */
