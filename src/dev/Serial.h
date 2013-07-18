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
#ifndef _Serial_h_
#define _Serial_h_ 1

#include "mach/platform.h"
#include "util/basics.h"

class SerialDevice0 { // see http://wiki.osdev.org/Serial_Ports
  friend class DebugDevice;
  friend class Machine;
  static const uint16_t SerialPort0 = 0x3F8;
  static bool gdb;

  static void init(bool g = true) {
    gdb = g;
    out8(SerialPort0 + 1, 0x00);    // Disable all interrupts
    out8(SerialPort0 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    out8(SerialPort0 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    out8(SerialPort0 + 1, 0x00);    //                  (hi byte)
    out8(SerialPort0 + 3, 0x03);    // 8 bits, no parity, one stop bit
    out8(SerialPort0 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
//    out8(SerialPort0 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }
  static void setGdb(bool g) {      // DBG::test(DBG::GDBEnable) not usable until multiboot::init
    gdb = g;
  }
public:
  static void write(char c) {
    while ((in8(SerialPort0 + 5) & 0x20) == 0);
    out8(SerialPort0, c);
    if likely(gdb || c != '\n') return;
    // inject LF to make serial output appear properly
    while ((in8(SerialPort0 + 5) & 0x20) == 0);
    out8(SerialPort0, '\r');
  }
  static char read() {
    while ((in8(SerialPort0 + 5) & 1) == 0);
    return in8(SerialPort0);
  }
};

class DebugDevice {
  friend class Machine;
  static bool valid;
  static void init() { valid = (in8(0xE9) == 0xE9); }
public:
  DebugDevice() = delete;                                  // no creation
  DebugDevice(const DebugDevice&) = delete;                // no copy
  DebugDevice& operator=(const DebugDevice&) = delete;     // no assignment
  static void write(char c) {
    if (valid) out8(0xE9, c);
    if (!SerialDevice0::gdb) SerialDevice0::write(c);
  }
};
  
#endif /* _Serial_h_ */
