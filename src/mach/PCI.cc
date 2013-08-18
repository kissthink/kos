/******************************************************************************
    Copyright 2012-2013 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "mach/PCI.h"
#include "mach/Bus.h"
#include "kern/Kernel.h"
#include "drivers/pci/pci_list.h"

#include <cstdio>

#define MAX_BUS 4

const BitSeg<uint32_t, 0,2> PCI::Config::Zero;
const BitSeg<uint32_t, 2,6> PCI::Config::Register;
const BitSeg<uint32_t, 8,3> PCI::Config::Function;
const BitSeg<uint32_t,11,5> PCI::Config::Device;
const BitSeg<uint32_t,16,8> PCI::Config::Bus;
const BitSeg<uint32_t,24,7> PCI::Config::Reserved;
const BitSeg<uint32_t,31,1> PCI::Config::Enable;

const BitSeg<uint32_t, 0,16> PCI::Vendor;
const BitSeg<uint32_t,16,16> PCI::Dev;

struct ConfigSpace {
  uint16_t Vendor;
  uint16_t Device;
  uint16_t Command;
  uint16_t Status;
  uint8_t  Revision;
  uint8_t  ProgramInterface;
  uint8_t  SubClass;
  uint8_t  ClassCode;
  uint8_t  CacheLineSize;
  uint8_t  LatencyTimer;
  uint8_t  HeaderType;
  uint8_t  Bist;
  uint32_t Bar[6];
  uint32_t CardBusPointer;
  uint16_t SubsysVendor;
  uint16_t SubsysId;
  uint32_t RomBaseAddr;
  uint32_t Reserved0;
  uint32_t Reserved1;
  uint8_t  InterruptLine;
  uint8_t  InterruptPin;
  uint8_t  MinGrant;
  uint8_t  MaxLatency;
} __packed;

static void readConfigSpace(ConfigSpace& cs, Device* dev) {
  uint32_t* pcs = reinterpret_cast<uint32_t*>(&cs);
  for (unsigned int i = 0; i < sizeof(ConfigSpace)/4; i++) {
    pcs[i] = PCI::readConfigWord32(dev, i);
  }
}

static const char* getVendor(uint16_t vendor) {
  for (unsigned int i = 0; i < PCI_VENTABLE_LEN; i++) {
    if (PCIVendorTable[i].VendorId == vendor)
      return PCIVendorTable[i].VendorIdShort;
  }
  return "";
}

static const char* getDevice(uint16_t vendor, uint16_t device) {
  for (unsigned int i = 0; i < PCI_DEVTABLE_LEN; i++) {
    if (PCIDeviceTable[i].VendorId == vendor && PCIDeviceTable[i].DeviceId == device)
      return PCIDeviceTable[i].ChipDesc;
  }
  return "";
}

void PCI::initDevices() {
  for (int busIdx = 0; busIdx < MAX_BUS; busIdx++) {
    // add ISA bus
    char* str = new char[256];
    sprintf(str, "PCI #%d", busIdx);
    Bus* bus = new Bus(str);
    bus->setSpecificType(kstring("pci"));

    for (int devIdx = 0; devIdx < 32; devIdx++) {
      bool isMultiFunc = false;
      for (int funcIdx = 0; funcIdx < 8; funcIdx++) {
        if (funcIdx > 0 && !isMultiFunc)
          break;

        Device* dev = new Device();
        dev->setPCIPosition(busIdx, devIdx, funcIdx);
        uint32_t vendorDeviceId = PCI::readConfigWord32(dev, 0);
        DBG::outln(DBG::PCI, "vendor & deviceId", FmtHex(vendorDeviceId));
        if ((vendorDeviceId & 0xFFFF) == 0xFFFF || (vendorDeviceId & 0xFFFF) == 0) {
          delete dev;
          continue;
        }

        ConfigSpace cs;
        readConfigSpace(cs, dev);

        if (cs.HeaderType & 0x80)
          isMultiFunc = true;

        DBG::outln(DBG::PCI, "PCI: ", busIdx, ':', devIdx, ':', funcIdx, "\t Vendor:", FmtHex(cs.Vendor), " Device:", cs.Device);
        DBG::outln(DBG::PCI, "VendorStr: ", getVendor(cs.Vendor), " DeviceStr: ", getDevice(cs.Vendor, cs.Device));
        char c[256];
        sprintf(c, "%s - %s", getDevice(cs.Vendor, cs.Device), getVendor(cs.Vendor));
        dev->setSpecificType(c);
        dev->setPCIIdentifiers(cs.ClassCode, cs.SubClass, cs.Vendor, cs.Device, cs.ProgramInterface);

        for (int i = 0; i < 6; i++) {
          // PCI-PCI bridges have a different layout for last 4 bars
          if ((cs.HeaderType & 0x7F) == 0x1 && i >= 2)
            break;
          if (cs.Bar[i] == 0)
            continue;

          // write BAR with 0xFFFFFFFF to discover the size of mapping that device requires
          uint16_t offset = (0x10 + i*4) >> 2;
          PCI::writeConfigWord32(dev, offset, 0xffffffff);
          uint32_t mask = PCI::readConfigWord32(dev, offset);
          PCI::writeConfigWord32(dev, offset, cs.Bar[i]);

          // work out how much space needed to fill that mask
          uint32_t size = ~(mask & 0xfffffff0) + 1;
          bool io = cs.Bar[i] & 0x1;
          if (io)
            size &= 0xffff;

          sprintf(c, "bar%d", i);
          uintptr_t s = cs.Bar[i] & 0xfffffff0;
          DBG::outln(DBG::PCI, "PCI Bar", i, ": ", FmtHex(s), "..", FmtHex(s+size), " (", io, ')');
          Device::Address* devAddr = new Device::Address(kstring(c), cs.Bar[i] & 0xfffffff0, size, (cs.Bar[i] & 0x1) == 0x1);
          dev->addresses().push_back(devAddr);
        }

        DBG::outln(DBG::PCI, "PCI IRQ Line:", cs.InterruptLine, " Pin:", cs.InterruptPin);
        dev->setInterruptNumber(cs.InterruptLine);
        bus->addChild(dev);
        dev->setParent(bus);
      }
    }

    if (bus->getNumChildren() > 0) {
      Device::root().addChild(bus);
      bus->setParent(&Device::root());
    } else{
      delete bus;
    }
  }
}
