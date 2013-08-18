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
#ifndef _PCI_h_
#define _PCI_h_ 1

#include "mach/platform.h"
#include "mach/Device.h"
#include "mach/IOPort.h"
#include "kern/Debug.h"

static IOPort configSpace("PCI Configuration Space");

class PCI {
  static const uint16_t AddressPort = 0xCF8;
  static const uint16_t DataPort    = 0xCFC;

  struct Config {
    static const BitSeg<uint32_t, 0,2> Zero;
    static const BitSeg<uint32_t, 2,6> Register;
    static const BitSeg<uint32_t, 8,3> Function;
    static const BitSeg<uint32_t,11,5> Device;
    static const BitSeg<uint32_t,16,8> Bus;
    static const BitSeg<uint32_t,24,7> Reserved;
    static const BitSeg<uint32_t,31,1> Enable;
  };

  static const int MaxBus    = pow2(8);
  static const int MaxDevice = pow2(5);

  static const BitSeg<uint32_t, 0,16> Vendor;
  static const BitSeg<uint32_t,16,16> Dev;  // avoid conflict with Device class

public:
  static void sanityCheck() {
    if (!configSpace.allocate(0xCF8, 8)) {
      ABORT1("PCI: Config space unable to allocate IO port!");
    }
    configSpace.write32(0x80000000, 0);
    if (configSpace.read32(0) != 0x80000000) {
      ABORT1("PCI: Controller not detected.");
    }
  }

  static uint32_t readConfigWord8(Device* dev, uint16_t reg) {
    return readConfigWord(dev->getPCIBusPosition(), dev->getPCIDevicePosition(), dev->getPCIFunctionNumber(), reg, 8);
  }
  static uint32_t readConfigWord16(Device* dev, uint16_t reg) {
    return readConfigWord(dev->getPCIBusPosition(), dev->getPCIDevicePosition(), dev->getPCIFunctionNumber(), reg, 16);
  }
  static uint32_t readConfigWord32(Device* dev, uint16_t reg) {
    return readConfigWord(dev->getPCIBusPosition(), dev->getPCIDevicePosition(), dev->getPCIFunctionNumber(), reg, 32);
  }

  static uint32_t readConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width) {
    uint32_t addr = Config::Enable() | Config::Bus(bus) | Config::Device(dev)
      | Config::Function(func) | Config::Register(reg);
    out32(AddressPort, addr);
    switch (width) {
      case 8:  return  in8(DataPort);
      case 16: return in16(DataPort);
      case 32: return in32(DataPort);
      default: ABORT1(width); return 0;
    }
  }

  static void writeConfigWord8(Device* dev, uint16_t reg, uint32_t value) {
    writeConfigWord(dev->getPCIBusPosition(), dev->getPCIDevicePosition(), dev->getPCIFunctionNumber(), reg, 8, value);
  }
  static void writeConfigWord16(Device* dev, uint16_t reg, uint32_t value) {
    writeConfigWord(dev->getPCIBusPosition(), dev->getPCIDevicePosition(), dev->getPCIFunctionNumber(), reg, 16, value);
  }
  static void writeConfigWord32(Device* dev, uint16_t reg, uint32_t value) {
    writeConfigWord(dev->getPCIBusPosition(), dev->getPCIDevicePosition(), dev->getPCIFunctionNumber(), reg, 32, value);
  }

  static void writeConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width, uint32_t value) {
    uint32_t addr = Config::Enable() | Config::Bus(bus) | Config::Device(dev)
      | Config::Function(func) | Config::Register(reg);
    out32(AddressPort, addr);
    switch (width) {
      case 8:   out8( DataPort, value ); break;
      case 16: out16( DataPort, value ); break;
      case 32: out32( DataPort, value ); break;
      default: ABORT1(width);
    }
  }

  static uint32_t probeDevice(uint16_t bus, uint16_t device) {
    return readConfigWord(bus, device, 0, 0, 32);
  }

  static void probeAll() {
    for (int bus = 0; bus < MaxBus; bus += 1) {
      for (int dev = 0; dev < MaxDevice; dev += 1) {
        uint32_t r = probeDevice(bus,dev);
        if (Vendor.get(r) != 0xFFFF) {
          DBG::outln(DBG::PCI, "PCI ", bus, '/', dev, ' ', FmtHex(Vendor.get(r),4), ':', FmtHex(Dev.get(r),4));
        }
      }
    }
  }

  static void initDevices();

};

#endif /* _PCI_h_ */
