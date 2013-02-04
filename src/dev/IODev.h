/******************************************************************************
    Copyright 2012 Martin Karsten

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
#ifndef _IODev_h_
#define _IODev_h_ 1

#include "Screen.h"

#include <streambuf>

using namespace std;

class OutputDevice {
public:
  virtual streamsize write(const char* s, streamsize n) { return 0; }
  virtual int sync() { return 0; }
};

class InputDevice {
public:
  virtual streamsize read(const char* s, streamsize n) { return 0; }
};

class SerialDevice : public OutputDevice, public InputDevice { // see http://wiki.osdev.org/Serial_Ports
  static const uint16_t SerialPort0 = 0x3F8;

public:
  SerialDevice() {
    out8(SerialPort0 + 1, 0x00);    // Disable all interrupts
    out8(SerialPort0 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    out8(SerialPort0 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    out8(SerialPort0 + 1, 0x00);    //                  (hi byte)
    out8(SerialPort0 + 3, 0x03);    // 8 bits, no parity, one stop bit
    out8(SerialPort0 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte thre
    //  outb(SerialPort0 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }
  virtual streamsize write(const char* s, streamsize n) {
    for (streamsize i = 0; i < n; i += 1) {
      while ((in8(SerialPort0 + 5) & 0x20) == 0);
      out8(SerialPort0, s[i]);
    }
    return n;
  }
  virtual streamsize read(char* s, streamsize n) {
    for (streamsize i = 0; i < n; i += 1 ) {
      while ((in8(SerialPort0 + 5) & 1) == 0);
      s[i] = in8(SerialPort0);
    }
    return n;
  }
};

class DebugDevice : public SerialDevice {
  bool valid;

public:
  DebugDevice() : valid(in8(0xE9) == 0xE9) {}
  virtual streamsize write(const char* s, streamsize n) {
    if (valid) for (streamsize i = 0; i < n; i += 1) out8(0xE9, s[i]);
    return n;
//    return SerialDevice::write(s, n);
  }
};

class ScreenSegment : public OutputDevice {
  int offset;
  int length;
  int position;

public:
  ScreenSegment(int firstline, int lastline, int startline = -1)
    : offset(Screen::offset(firstline)),
      length(Screen::length(firstline,lastline)) {
    position = (startline > firstline) ? Screen::offset(startline) : Screen::offset(firstline);
  }
  ScreenSegment(const ScreenSegment&) = delete;            // no copy
  ScreenSegment& operator=(const ScreenSegment&) = delete; // no assignment
  void cls() { Screen::cls( offset, length ); }
  virtual streamsize write(const char* s, streamsize n) {
    for (streamsize i = 0; i < n; i += 1) {
      Screen::putchar( s[i], position, offset, length );
    }
    return n;
  }
};

#endif /* _IODev_h_ */
