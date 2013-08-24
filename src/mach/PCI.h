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
#include "mach/IOPort.h"
#include "mach/IOPortManager.h"
#include "kern/Debug.h"

class Bus;
class PCI {
  friend class PCIDevice;
  static const uint16_t AddressPort = 0xCF8;
  static const uint16_t DataPort    = 0xCFC;
  static IOPort addressPort;
  static IOPort dataPort;

  struct Config {
    static const BitSeg<uint32_t, 0,2> Zero;
    static const BitSeg<uint32_t, 2,6> Register;
    static const BitSeg<uint32_t, 8,3> Function;
    static const BitSeg<uint32_t,11,5> Device;
    static const BitSeg<uint32_t,16,8> Bus;
    static const BitSeg<uint32_t,24,7> Reserved;
    static const BitSeg<uint32_t,31,1> Enable;
  };

  struct PCIGeneral {
    static const BitSeg<uint32_t, 0,16> SubVendorID;
    static const BitSeg<uint32_t,16,16> SubSystemID;
    static const BitSeg<uint32_t, 0, 8> Capabilities;
    static const BitSeg<uint32_t, 0, 8> InterruptLine;
    static const BitSeg<uint32_t, 8, 8> InterruptPin;
    static const BitSeg<uint32_t,16, 8> MinGrant;
    static const BitSeg<uint32_t,24, 8> MaxLatency;
  };

  struct PCItoPCIBridge {
    static const BitSeg<uint32_t, 0, 8> PrimaryBus;
    static const BitSeg<uint32_t, 8, 8> SecondaryBus;
    static const BitSeg<uint32_t,16, 8> SubordinateBus;
    static const BitSeg<uint32_t,24, 8> SecondaryLatencyTimer;
    static const BitSeg<uint32_t, 0, 8> IOBase;
    static const BitSeg<uint32_t, 8, 8> IOLimit;
    static const BitSeg<uint32_t,16,16> SecondaryStatus;
    static const BitSeg<uint32_t, 0,16> MemoryBase;
    static const BitSeg<uint32_t,16,16> MemoryLimit;
    static const BitSeg<uint32_t, 0,16> PrefetchBase;
    static const BitSeg<uint32_t,16,16> PrefetchLimit;
    static const BitSeg<uint32_t, 0,16> IOBaseUpper16;
    static const BitSeg<uint32_t,16,16> IOLimitUpper16;
    static const BitSeg<uint32_t, 0, 8> Capabilities;
    static const BitSeg<uint32_t, 0, 8> InterruptLine;
    static const BitSeg<uint32_t, 8, 8> InterruptPin;
    static const BitSeg<uint32_t,16,16> BridgeControl;
  };

  struct PCItoCardBusBridge {
    static const BitSeg<uint32_t, 0, 8> CapabilityOffset;
    static const BitSeg<uint32_t,16,16> SecondaryStatus;
    static const BitSeg<uint32_t, 0, 8> PCIBusNum;
    static const BitSeg<uint32_t, 8, 8> CardBusNum;
    static const BitSeg<uint32_t,16, 8> SubordinateBusNum;
    static const BitSeg<uint32_t,24, 8> CardBusLatencyTimer;
    static const BitSeg<uint32_t, 0, 8> InterruptLine;
    static const BitSeg<uint32_t, 8, 8> InterruptPin;
    static const BitSeg<uint32_t,16,16> BridgeControl;
    static const BitSeg<uint32_t, 0,16> SubDeviceID;
    static const BitSeg<uint32_t,16,16> SubVendorID;
  };

  static const int MaxBus    = pow2(8);
  static const int MaxDevice = pow2(5);

  static const BitSeg<uint32_t, 0,16> VendorID;
  static const BitSeg<uint32_t,16,16> DeviceID;
  static const BitSeg<uint32_t, 0,16> Command;
  static const BitSeg<uint32_t,16,16> Status;
  static const BitSeg<uint32_t, 0, 8> Revision;
  static const BitSeg<uint32_t, 8, 8> ProgIF;
  static const BitSeg<uint32_t,16, 8> SubClass;
  static const BitSeg<uint32_t,24, 8> ClassCode;
  static const BitSeg<uint32_t, 0, 8> CacheLineSize;
  static const BitSeg<uint32_t, 8, 8> LatencyTimer;
  static const BitSeg<uint32_t,16, 8> HeaderType;
  static const BitSeg<uint32_t,24, 8> BIST;

public:
  static void sanityCheck() {
    if (!IOPortManager::allocate(addressPort, AddressPort, 4)) {
      ABORT1("PCI: config space unable to reserve IO port!");
    }
    if (!IOPortManager::allocate(dataPort, DataPort, 4)) {
      ABORT1("PCI: config space unable to reserve IO port!");
    }
    addressPort.write32(0, 0x80000000);
    if (addressPort.read32(0) != 0x80000000) {
      ABORT1("PCI: controller not detected");
    }
  }

  static uint32_t readConfigWord(uint16_t bus, uint16_t dev, uint16_t func, uint16_t reg, uint32_t width) {
    uint32_t addr = Config::Enable() | Config::Bus(bus) | Config::Device(dev)
      | Config::Function(func) | Config::Register(reg);
    addressPort.write32( 0, addr );
    switch (width) {
      case 8:  return dataPort.read8( 0 );
      case 16: return dataPort.read16( 0 );
      case 32: return dataPort.read32( 0 );
      default: ABORT1(width); return 0;
    }
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
        if (VendorID.get(r) != 0xFFFF) {
          DBG::outln(DBG::PCI, "PCI ", bus, '/', dev, ' ', FmtHex(VendorID.get(r),4), ':', FmtHex(DeviceID.get(r),4));
        }
      }
    }
  }

  static void checkDevice(uint8_t bus, uint8_t device, Bus* busObj);
  static void checkBus(uint8_t bus);
  static void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
  static void checkAllBuses();

};

#endif /* _PCI_h_ */
