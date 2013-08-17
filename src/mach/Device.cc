#include "mach/Device.h"
#include "mach/IOPort.h"
#include "mach/MemoryMappedIO.h"
#include "kern/Debug.h"

Device Device::rootDevice;

Device::Device() : parentDevice(0), interruptNumber(0), pciClassCode(0), pciSubClassCode(0), pciVendorId(0), pciDeviceId(0), pciProgIf(0),
    pciBusPos(0), pciDevicePos(0), pciFuncNum(0) {}
Device::Device(Device* p) : parentDevice(0), interruptNumber(p->interruptNumber), specificType(p->specificType),
    pciClassCode(p->pciClassCode), pciSubClassCode(p->pciSubClassCode), pciVendorId(p->pciVendorId), pciDeviceId(p->pciDeviceId),
    pciProgIf(p->pciProgIf), pciBusPos(p->pciBusPos), pciDevicePos(p->pciDevicePos), pciFuncNum(p->pciFuncNum)
{
  parentDevice = p->parentDevice;
  for (Device* d : p->devChildren) {
    devChildren.push_back(d);
  }
  p->removeIOMappings();
  for (Address* pa : p->devAddresses) {
    Address* a = new Address(pa->addrName, pa->baseAddr, pa->addrSize, pa->ioSpace, pa->addrPadding);
    DBG::outln(DBG::Devices, "address=", FmtHex(reinterpret_cast<ptr_t>(a)), ", IO=", FmtHex(reinterpret_cast<ptr_t>(a->ioAddress)));
    devAddresses.push_back(a);
  }
}

Device::~Device() {
  while (!devAddresses.empty()) {
    Address* a = devAddresses.front();
    devAddresses.pop_front();
    delete a;
  }
  while (!devChildren.empty()) {
    Device* d = devChildren.front();
    devChildren.pop_front();
    delete d;
  }
}

void Device::removeIOMappings() {
  for (Address* addr : devAddresses) {
    if (addr->ioAddress)
      delete addr->ioAddress;
    addr->ioAddress = nullptr;
  }
}

void Device::getName(kstring& str) {
  str = "Root";
}

void Device::addChild(Device* p) {
  devChildren.push_back(p);
}

Device* Device::getChild(mword n) {
  mword i = 0;
  for (Device* p : devChildren) {
    if (i == n) {
      return p;
    }
    ++i;
  }
  return nullptr;
}

mword Device::getNumChildren() {
  return devChildren.size();
}

void Device::removeChild(mword n) {
  mword i = 0;
  typedef list<Device*,KernelAllocator<Device*>> DeviceList;
  for (DeviceList::iterator it = devChildren.begin();
      it != devChildren.end();
      ++it, ++i) {
    if (i == n) {
      devChildren.erase(it);
      break;
    }
  }
}

void Device::removeChild(Device* p) {
  typedef list<Device*,KernelAllocator<Device*>> DeviceList;
  for (DeviceList::iterator it = devChildren.begin();
      it != devChildren.end();
      ++it) {
    if ((*it) == p) {
      devChildren.erase(it);
      break;
    }
  }
}

void Device::replaceChild(Device* src, Device* dst) {
  typedef list<Device*,KernelAllocator<Device*>> DeviceList;
  for (DeviceList::iterator it = devChildren.begin();
      it != devChildren.end();
      ++it) {
    if ((*it) == src) {
      *it = dst;
      break;
    }
  }
}

void Device::searchByVendorId(uint16_t vendorId, void (*callback)(Device*)) {
  for (Device* p : devChildren) {
    if (p->getPCIVendorId() == vendorId)
      callback(p);
    p->searchByVendorId(vendorId, callback);
  }
}

void Device::searchByVendorIdAndDeviceId(uint16_t vendorId, uint16_t deviceId, void (*callback)(Device*)) {
  for (Device* p : devChildren) {
    if (p->getPCIVendorId() == vendorId && p->getPCIDeviceId() == deviceId)
      callback(p);
    p->searchByVendorIdAndDeviceId(vendorId, deviceId, callback);
  }
}

void Device::searchByClass(uint16_t classCode, void (*callback)(Device*)) {
  for (Device* p : devChildren) {
    if (p->getPCIClassCode() == classCode)
      callback(p);
    p->searchByClass(classCode, callback);
  }
}

void Device::searchByClassAndSubClass(uint16_t classCode, uint16_t subClassCode, void (*callback)(Device*)) {
  for (Device* p : devChildren) {
    if (p->getPCIClassCode() == classCode && p->getPCISubClassCode() == subClassCode)
      callback(p);
    p->searchByClassAndSubClass(classCode, subClassCode, callback);
  }
}

void Device::searchByClassSubClassAndProgInterface(uint16_t classCode, uint16_t subClassCode, uint8_t progIf, void (*callback)(Device*)) {
  for (Device* p : devChildren) {
    if (p->getPCIClassCode() == classCode && p->getPCISubClassCode() == subClassCode && p->getPCIProgInterface() == progIf)
      callback(p);
    p->searchByClassSubClassAndProgInterface(classCode, subClassCode, progIf, callback);
  }
}

Device::Address::Address(kstring& n, uintptr_t a, mword s, bool io, mword pad)
  : isMapped(false), addrName(n), baseAddr(a), addrSize(s), ioSpace(io), addrPadding(pad)
{
  if (ioSpace) {
    IOPort* ioPort = new IOPort(addrName.c_str());
    ioPort->allocate(a, s);
    ioAddress = ioPort;
  } else {
    ABORT1("implement memory mapped IO");
  }
}

void Device::Address::map(mword forcedSize, bool bUser) {
  ABORT1("implement memory mapped IO");
}

Device::Address::~Address() {
  if (ioAddress)
    delete ioAddress;
}
