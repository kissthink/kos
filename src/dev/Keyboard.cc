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
/* details taken from http://wiki.osdev.org/%228042%22_PS/2_Controller */

#include "mach/APIC.h"
#include "dev/Keyboard.h"

static inline void writectl(uint8_t val) {
  out8(0x64,val);
}

static inline uint8_t readctl() {
  return in8(0x64);
}

static inline void writedata(uint8_t val) {
  while (readctl() & 0x02);           // wait until buffer is clear
  out8(0x60, val);
}

static inline uint8_t readdata() {
  while (!(readctl() & 0x01));        // wait until data arrives
  return in8(0x60);
}

static inline uint8_t assertdata() {
  KASSERT(readctl() & 0x01, "no keyboard scan code");
  return in8(0x60);
}

static inline bool testdata(uint8_t &data) {
  if (readctl() & 0x01) {
    data = in8(0x60);
    return true;
  } else {
    return false;
  }
}

// translate scan codes into "standard" X11 keycodes? (see xmodmap -pk)
static int scanCodeSet = 0;
static const int maxScanCode = 0x80;
static const uint8_t keyCode[2][maxScanCode] = {
{   0,   9,  10,  11,  12,  13,  14,  15, 
   16,  17,  18,  19,  20,  21,  22,  23, 
   24,  25,  26,  27,  28,  29,  30,  31, 
   32,  33,  34,  35,  36,  37,  38,  39, 
   40,  41,  42,  43,  44,  45,  46,  47, 
   48,  49,  50,  51,  52,  53,  54,  55, 
   56,  57,  58,  59,  60,  61,  62,  63, 
   64,  65,  66,  67,  68,  69,  70,  71, 
   72,  73,  74,  75,  76,  77,  78,  79, 
   80,  81,  82,  83,  84,  85,  86,  87, 
   88,  89,  90,  91,   0,   0,   0,  95, 
   96,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0 },
{   0,  75,   0,  71,  69,  67,  68,  96, 
    0,  76,  74,  72,  70,  23,  49,   0, 
    0,  64,  50,   0,  37,  24,  10,   0, 
    0,   0,  52,  39,  38,  25,  11,   0, 
    0,  54,  53,  40,  26,  13,  12,   0, 
    0,  65,  55,  41,  28,  27,  14,   0, 
    0,  57,  56,  43,  42,  29,  15,   0, 
    0,   0,  58,  44,  30,  16,  17,   0, 
    0,  59,  45,  31,  32,  19,  18,   0, 
    0,  60,  61,  46,  47,  33,  20,   0, 
    0,   0,  48,   0,  34,  21,   0,   0, 
   66,  62,  36,  35,   0,  51,   0,   0,
    0,   0,  22,   0,   0,  87,   0,  83,
   79,   0,   0,   0,  90,  91,  88,  84,
   85,  80,   9,  77,  95,  86,  89,  82,
   63,  81,  78,   0,   0,   0,   0,  73 }
};

bool Keyboard::init() {
  // initialize USB controllers -> not yet
  // determine PS/2 presence -> done in ACPI, but not supported in qemu/bochs

  uint8_t config, data;

  writectl(0xAD);                     // disable 1st PS/2 port
  writectl(0xA7);                     // disable 2nd PS/2 port

  while (readctl() & 0x01) in8(0x60); // clear keyboard buffer

  DBG::out(DBG::Devices, "PS/2 ports:");

  writectl(0x20);                     // ask for configuration byte
  config = readdata();                // read configuration byte
  config &= ~(1<<0);                  // disable 1st port interrupts
  config &= ~(1<<1);                  // disable 2nd port interrupts
  config &= ~(1<<6);                  // disable 1st port translation
  writectl(0x60);                     // announce configuration byte
  writedata(config);                  // write back configuration byte

  while (readctl() & 0x01) in8(0x60); // clear keyboard buffer

  writectl(0xAA);                     // perform self-test
  data = readdata();
  KASSERT( data == 0x55, (ptr_t)uintptr_t(data) );

  writectl(0xAE);                     // try enable 1st port
  writectl(0x20);                     // request configuration byte
  config = readdata();
  if (!(config & (1<<4))) {           // test for 1st port
    DBG::out(DBG::Devices, " 1st");
    writectl(0xAB);                   // test 1st port
    data = readdata();
    KASSERT( data == 0x00, (ptr_t)uintptr_t(data) );
    config |= (1<<0);                 // enable 1st port interrupts
    writectl(0x60);                   // write back configuration byte
    writedata(config);
    writedata(0xFF);                  // reset 1st device
    data = readdata();
    KASSERT( data == 0xFA, (ptr_t)uintptr_t(data) );
    if (testdata(data)) KASSERT( data == 0xAA, (ptr_t)uintptr_t(data) ); // bochs/qemu quirk
  }

#if 1
  writectl(0xA8);                     // try enable 2nd port
  writectl(0x20);                     // request configuration byte
  config = readdata();
  if (!(config & (1<<5))) {           // test for 2nd port
    DBG::out(DBG::Devices, " 2nd"); 
    writectl(0xA9);                   // test 2nd port -> bochs panics when
    data = readdata();                //       also using 0xE9 debug output
    KASSERT( data == 0x00, (ptr_t)uintptr_t(data) );
    config |= (1<<1);                 // enable 2nd port interrupts
    writectl(0x60);                   // write back configuration byte
    writedata(config);
    writectl(0xD4);                   // send next byte to 2nd device
    writedata(0xFF);                  // reset 2nd device
    data = readdata();
    if (data == 0xAA) data = readdata(); // bochs quirk
    KASSERT( data == 0xFA, (ptr_t)uintptr_t(data) );
    if (testdata(data)) KASSERT( data == 0xAA, (ptr_t)uintptr_t(data) ); // bochs/qemu quirk
  }
#endif

  writedata(0xF0);                    // get/set scan code set
  data = readdata();
  KASSERT( data == 0xFA || data == 0, (ptr_t)uintptr_t(data) ); // bochs sends 0x00 instead of ACK
  writedata(0x01);                    // set scan code set 1
  data = readdata();
  KASSERT( data == 0xFA || data == 0xAA, (ptr_t)uintptr_t(data) ); // bochs sends 0xAA instead of ACK

  writedata(0xF0);                    // get/set scan code set
  data = readdata();
  KASSERT( data == 0xFA, (ptr_t)uintptr_t(data) );
  writedata(0x00);                    // get scan code set
  data = readdata();
  if (data == 0xFA) data = readdata(); // bochs sends extra ACK
  if (data == 0xFA) data = readdata(); // bochs sends extra ACK
  if (data == 0x02) scanCodeSet = 1;

  writectl(0x20);                     // request configuration byte
  config = readdata();
  DBG::outln(DBG::Devices, ' ', (ptr_t)mword(config), ' ', (ptr_t)mword(scanCodeSet));

  while (readctl() & 0x01) in8(0x60); // clear keyboard buffer

  Machine::staticEnableIRQ( PIC::Keyboard, 0x21 );
  return true;
}

// TODO: control LEDs
void Keyboard::staticInterruptHandler() {
  while (readctl() & 0x01) {
    uint8_t scanCode = in8(0x60);
    uint16_t kc;
    if (scanCodeSet == 0) {
      if (scanCode & 0x80) {
        kc = 0xFF00 | keyCode[scanCodeSet][(scanCode & ~0x80)];
      } else {
        kc = keyCode[scanCodeSet][scanCode];
      }
    } else {
      if (scanCode == 0xF0) {
        kc = 0xFF00 | keyCode[scanCodeSet][assertdata()];
      } else {
        kc = keyCode[scanCodeSet][scanCode];
      }
    }
    kcq.appendISR(kc);
  }
}
