#ifndef _Device_h_
#define _Device_h_ 1

#include "kern/Kernel.h"
#include "mach/IOBase.h"
#include "mach/platform.h"
#include <list>

class Device {
  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  void removeIOMappings();  // destroys all IOBases in this device

public:
  enum Type {
    Generic,
    Root,
    Disk,
    Bus,
    Display,
    Network,
    Sound,
    Console,
    Mouse,
    Controller,
    UsbController,
    UsbGeneric,
  };

  class Address { // each device may have one or more disjoint regions of addrspace
    Address(const Address&) = delete;
    Address& operator=(const Address&) = delete;

    bool isMapped;
  public:
    Address(kstring n, uintptr_t a, mword s, bool io, mword pad=1);
    ~Address();
    void map(mword forcedSize = 0, bool bUser = false); // maps address into memory
    kstring addrName;   // address range identifier
    uintptr_t baseAddr; // base of address range as the processor sees it
    mword addrSize;     // address range
    bool ioSpace;
    IOBase* ioAddress;  // memory mapped I/O or port I/O
    mword addrPadding;  // some device's registers are padded to boundaries
  };
  
  Device();
  Device(Device* p);  // renders 'p' unusable by deleting all its IOPorts and MemoryMapped IOs
  virtual ~Device();

  static Device& root() {
    return rootDevice;
  }
  Device* getParent() {
    return parentDevice;
  }
  void setParent(Device* p) {
    parentDevice = p;
  }
  virtual void getName(kstring& str);
  virtual Type getType() {
    return Root;
  }
  virtual kstring getSpecificType() {
    return specificType;
  }
  virtual void setSpecificType(kstring str) {
    specificType = str;
  }
  void setPCIPosition(uint32_t bus, uint32_t device, uint32_t func) {
    pciBusPos = bus;
    pciDevicePos = device;
    pciFuncNum = func;
  }
  void setPCIIdentifiers(uint8_t classCode, uint8_t subclassCode, uint16_t vendorId, uint16_t deviceId, uint8_t progIf) {
    pciClassCode = classCode;
    pciSubClassCode = subclassCode;
    pciVendorId = vendorId;
    pciDeviceId = deviceId;
    pciProgIf = progIf; // program interface
  }
  uint8_t getPCIClassCode() {
    return pciClassCode;
  }
  uint8_t getPCISubClassCode() {
    return pciSubClassCode;
  }
  uint16_t getPCIVendorId() {
    return pciVendorId;
  }
  uint16_t getPCIDeviceId() {
    return pciDeviceId;
  }
  uint8_t getPCIProgInterface() {
    return pciProgIf;
  }
  uint32_t getPCIBusPosition() {
    return pciBusPos;
  }
  uint32_t getPCIDevicePosition() {
    return pciDevicePos;
  }
  uint32_t getPCIFunctionNumber() {
    return pciFuncNum;
  }
  virtual void dump(kstring& str) {
    str = "Abstract Device";
  }
  virtual std::list<Address*,KernelAllocator<Address*>>& addresses() {
    return devAddresses;
  }
  virtual mword getInterruptNumber() {
    return interruptNumber;
  }
  virtual void setInterruptNumber(mword n) {
    interruptNumber = n;
  }

  void addChild(Device* p); // p's parent is not updated
  Device* getChild(mword n);  // returns nth child of this device
  mword getNumChildren();
  void removeChild(mword n);
  void removeChild(Device *d);
  void replaceChild(Device* src, Device* dest);

  void searchByVendorId(uint16_t vendorId, void (*callback)(Device*));
  void searchByVendorIdAndDeviceId(uint16_t vendorId, uint16_t deviceId, void (*callback)(Device*));
  void searchByClass(uint16_t classCode, void (*callback)(Device*));
  void searchByClassAndSubClass(uint16_t classCode, uint16_t subClassCode, void (*callback)(Device*));
  void searchByClassSubClassAndProgInterface(uint16_t classCode, uint16_t subClassCode, uint8_t progInterface, void (*callback)(Device*));

protected:
  std::list<Address*,KernelAllocator<Address*>> devAddresses;
  std::list<Device*,KernelAllocator<Device*>> devChildren;
  Device* parentDevice;
  static Device rootDevice;
  mword interruptNumber;
  kstring specificType;

  uint8_t pciClassCode;
  uint8_t pciSubClassCode;
  uint16_t pciVendorId;
  uint16_t pciDeviceId;
  uint8_t pciProgIf;
  uint32_t pciBusPos;
  uint32_t pciDevicePos;
  uint32_t pciFuncNum;
};

#endif /* _Device_h_ */
