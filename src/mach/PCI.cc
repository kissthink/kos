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
#include "mach/PCIHeader.h"
#include "mach/PCIDevice.h"
#include "mach/PCIBus.h"

IOPort PCI::addressPort("PCI ConfigSpace Address Port");
IOPort PCI::dataPort("PCI ConfigSpace Data Port");

const BitSeg<uint32_t, 0,2> PCI::Config::Zero;
const BitSeg<uint32_t, 2,6> PCI::Config::Register;
const BitSeg<uint32_t, 8,3> PCI::Config::Function;
const BitSeg<uint32_t,11,5> PCI::Config::Device;
const BitSeg<uint32_t,16,8> PCI::Config::Bus;
const BitSeg<uint32_t,24,7> PCI::Config::Reserved;
const BitSeg<uint32_t,31,1> PCI::Config::Enable;

// common fields
// reg 0
const BitSeg<uint32_t, 0,16> PCI::VendorID;       // identifies manufacturer
const BitSeg<uint32_t,16,16> PCI::DeviceID;       // identifies device
// reg 1
const BitSeg<uint32_t, 0,16> PCI::Command;        // functionalities
const BitSeg<uint32_t,16,16> PCI::Status;         // PCI bus related status
// reg 2
const BitSeg<uint32_t, 0, 8> PCI::Revision;       // revision ID
const BitSeg<uint32_t, 8, 8> PCI::ProgIF;         // register-level programming interface
const BitSeg<uint32_t,16, 8> PCI::SubClass;       // specific function device performs
const BitSeg<uint32_t,24, 8> PCI::ClassCode;      // type of function device performs
// reg 3
const BitSeg<uint32_t, 0, 8> PCI::CacheLineSize;  // system cache line size in 2-byte units
const BitSeg<uint32_t, 8, 8> PCI::LatencyTimer;   // latency timer in units of PCI bus clock
const BitSeg<uint32_t,16, 8> PCI::HeaderType;     // layout of rest of config space + multi-function
const BitSeg<uint32_t,24, 8> PCI::BIST;           // status + control of a device's built-in self test

  // reg 11
const BitSeg<uint32_t, 0,16> PCI::PCIGeneral::SubVendorID;
const BitSeg<uint32_t,16,16> PCI::PCIGeneral::SubSystemID;
  // reg 13
const BitSeg<uint32_t, 0, 8> PCI::PCIGeneral::Capabilities;   // points to linked list of new capabilities of device
  // reg 15
const BitSeg<uint32_t, 0, 8> PCI::PCIGeneral::InterruptLine;  // NOT I/O APIC IRQ numbers
const BitSeg<uint32_t, 8, 8> PCI::PCIGeneral::InterruptPin;   // specifies interrupt pin device uses (A,B,C,D)
const BitSeg<uint32_t,16, 8> PCI::PCIGeneral::MinGrant;       // burst period length in 1/4 microsec
const BitSeg<uint32_t,24, 8> PCI::PCIGeneral::MaxLatency;     // how often the device needs access to PCI bus in 1/4 microsec

  // reg 6
const BitSeg<uint32_t, 0, 8> PCI::PCItoPCIBridge::PrimaryBus;
const BitSeg<uint32_t, 8, 8> PCI::PCItoPCIBridge::SecondaryBus;
const BitSeg<uint32_t,16, 8> PCI::PCItoPCIBridge::SubordinateBus;
const BitSeg<uint32_t,24, 8> PCI::PCItoPCIBridge::SecondaryLatencyTimer;
  // reg 7
const BitSeg<uint32_t, 0, 8> PCI::PCItoPCIBridge::IOBase;
const BitSeg<uint32_t, 8, 8> PCI::PCItoPCIBridge::IOLimit;
const BitSeg<uint32_t,16,16> PCI::PCItoPCIBridge::SecondaryStatus;
  // reg 8
const BitSeg<uint32_t, 0,16> PCI::PCItoPCIBridge::MemoryBase;
const BitSeg<uint32_t,16,16> PCI::PCItoPCIBridge::MemoryLimit;
  // reg 9
const BitSeg<uint32_t, 0,16> PCI::PCItoPCIBridge::PrefetchBase;
const BitSeg<uint32_t,16,16> PCI::PCItoPCIBridge::PrefetchLimit;
  // reg 12
const BitSeg<uint32_t, 0,16> PCI::PCItoPCIBridge::IOBaseUpper16;
const BitSeg<uint32_t,16,16> PCI::PCItoPCIBridge::IOLimitUpper16;
  // reg 13
const BitSeg<uint32_t, 0, 8> PCI::PCItoPCIBridge::Capabilities;
  // reg 15
const BitSeg<uint32_t, 0, 8> PCI::PCItoPCIBridge::InterruptLine;
const BitSeg<uint32_t, 8, 8> PCI::PCItoPCIBridge::InterruptPin;
const BitSeg<uint32_t,16,16> PCI::PCItoPCIBridge::BridgeControl;

  // reg 5
const BitSeg<uint32_t, 0, 8> PCI::PCItoCardBusBridge::CapabilityOffset;
const BitSeg<uint32_t,16,16> PCI::PCItoCardBusBridge::SecondaryStatus;
  // reg 6
const BitSeg<uint32_t, 0, 8> PCI::PCItoCardBusBridge::PCIBusNum;
const BitSeg<uint32_t, 8, 8> PCI::PCItoCardBusBridge::CardBusNum;
const BitSeg<uint32_t,16, 8> PCI::PCItoCardBusBridge::SubordinateBusNum;
const BitSeg<uint32_t,24, 8> PCI::PCItoCardBusBridge::CardBusLatencyTimer;
  // reg 15
const BitSeg<uint32_t, 0, 8> PCI::PCItoCardBusBridge::InterruptLine;
const BitSeg<uint32_t, 8, 8> PCI::PCItoCardBusBridge::InterruptPin;
const BitSeg<uint32_t,16,16> PCI::PCItoCardBusBridge::BridgeControl;
  // reg 16
const BitSeg<uint32_t, 0,16> PCI::PCItoCardBusBridge::SubDeviceID;
const BitSeg<uint32_t,16,16> PCI::PCItoCardBusBridge::SubVendorID;

const char* PCI::getVendorName(uint16_t vendorID) {
  for (unsigned int i = 0; i < PCI_VENTABLE_LEN; i++) {
    if (PCIVendorTable[i].VendorId == vendorID) {
      return PCIVendorTable[i].VendorFull;
    }
  }
  return "Unknown vendor";
}

const char* PCI::getDeviceName(uint16_t vendorID, uint16_t deviceID) {
  for (unsigned int i = 0; i < PCI_DEVTABLE_LEN; i++) {
    if (PCIDeviceTable[i].VendorId == vendorID && PCIDeviceTable[i].DeviceId == deviceID) {
      return PCIDeviceTable[i].ChipDesc;
    }
  }
  return "Unknown device";
}

/**
 * Below functions are from http://wiki.osdev.org/PCI#Configuration_Mechanism_.231
 */

// check if a specific device on a specific bus is present
void PCI::checkDevice(uint8_t bus, uint8_t device, Bus* busObj) {
  uint8_t func = 0;
  uint16_t vendorID = VendorID.get( readConfigWord(bus, device, func, 0, 32) );
  if (vendorID == 0xffff) return;    // device doesn't exist
  checkFunction(bus, device, func, busObj);
  uint8_t headerType = HeaderType.get( readConfigWord(bus, device, func, 3, 32) );
  if ((headerType & 0x80) != 0) {   // multi-function
    for (func = 1; func < 8; func++) {
      if (VendorID.get( readConfigWord(bus, device, func, 0, 32) ) != 0xffff) {
        checkFunction(bus, device, func, busObj);
      }
    }
  }
}

// scans one bus
void PCI::checkBus(uint8_t bus) {
  DBG::outln(DBG::PCI, "Checking BUS ", FmtHex(bus));
  Bus* busObj = new PCIBus(bus);
  Device::getRoot()->addChild(busObj);
  busObj->setParent(Device::getRoot());
  for (uint8_t device = 0; device < 32; device++) {
    checkDevice(bus, device, busObj);
  }
}

// detects if the function is a PCI to PCI bridge; also add the detected PCI device
void PCI::checkFunction(uint8_t bus, uint8_t device, uint8_t func, Bus* busObj) {
  uint8_t baseClass;
  uint8_t subClass;
  uint8_t secondaryBus;

  uint16_t vendorID = VendorID.get( readConfigWord(bus, device, func, 0, 32) );
  uint16_t deviceID = DeviceID.get( readConfigWord(bus, device, func, 0, 32) );
  uint16_t command = Command.get( readConfigWord(bus, device, func, 1, 32) );
  uint16_t status = Status.get( readConfigWord(bus, device, func, 1, 32) );
  uint32_t bar0 = readConfigWord(bus, device, func, 4, 32);
  uint32_t bar1 = readConfigWord(bus, device, func, 5, 32);
  uint8_t intLine = PCIGeneral::InterruptLine.get(readConfigWord(bus, device, func, 15, 32));
  uint8_t intPin = PCIGeneral::InterruptPin.get(readConfigWord(bus, device, func, 15, 32));
  uint8_t headerType = HeaderType.get( readConfigWord(bus, device, func, 3, 32) );
  DBG::outln(DBG::PCI, "bus: ", FmtHex(bus), " device: ", FmtHex(device), " func: ", FmtHex(func));
  DBG::outln(DBG::PCI, "VendorID: ", FmtHex(vendorID), " DeviceID: ", FmtHex(deviceID));
  DBG::outln(DBG::PCI, "Vendor: ", getVendorName(vendorID), " Device: ", getDeviceName(vendorID, deviceID));
  DBG::outln(DBG::PCI, "Command: ", FmtHex(command), " Status: ", FmtHex(status));
  DBG::outln(DBG::PCI, "BAR0: ", FmtHex(bar0));
  DBG::outln(DBG::PCI, "BAR1: ", FmtHex(bar1));
  if ((headerType & 0x7f) == 0) {
    DBG::outln(DBG::PCI, "BAR2: ", FmtHex(readConfigWord(bus, device, func, 6, 32)));
    DBG::outln(DBG::PCI, "BAR3: ", FmtHex(readConfigWord(bus, device, func, 7, 32)));
    DBG::outln(DBG::PCI, "BAR4: ", FmtHex(readConfigWord(bus, device, func, 8, 32)));
    DBG::outln(DBG::PCI, "BAR5: ", FmtHex(readConfigWord(bus, device, func, 9, 32)));
  } 
  DBG::outln(DBG::PCI, "Interrupt Line: ", FmtHex(intLine));
  DBG::outln(DBG::PCI, "Interrupt Pin: ", FmtHex(intPin));
  Device* dev = new PCIDevice( bus, device, func, getDeviceName(vendorID, deviceID) );
  busObj->addChild(dev);
  dev->setParent(busObj);

  baseClass = ClassCode.get( readConfigWord(bus, device, func, 2, 32) );
  subClass = SubClass.get( readConfigWord(bus, device, func, 2, 32) );
  if ( baseClass == 0x06 && subClass == 0x04 ) {
    secondaryBus = PCItoPCIBridge::SecondaryBus.get( readConfigWord(bus, device, func, 6, 32) );
    checkBus(secondaryBus); // recurse for secondary bus
  }
}

// handle multiple PCI host controllers
void PCI::checkAllBuses() {
  uint8_t headerType = HeaderType.get( readConfigWord(0,0,0,3,32) );
  if ((headerType & 0x80) == 0) {               // only one PCI host controller
    checkBus(0);
  } else {
    for (uint8_t func = 0; func < 8; func++) {  // check all possible controllers;
      if (VendorID.get(readConfigWord(0, 0, func, 0, 32)) != 0xffff) break;
      checkBus(func);
    }
  }
}
