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

#include "util/Debug.h"
#include "mach/platform.h"

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
  static const BitSeg<uint32_t,16,16> Device;

public:

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
          DBG::outln(DBG::PCI, "PCI ", bus, '/', dev, ' ', FmtHex(Vendor.get(r),4), ':', FmtHex(Device.get(r),4));
        }
      }
    }
  }

};

#endif /* _PCI_h_ */
