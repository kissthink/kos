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
#include "mach/APIC.h"
#include "mach/Machine.h"
#include "dev/RTC.h"

void RTC::init() volatile { // see http://wiki.osdev.org/RTC
  Machine::staticEnableIRQ( PIC::RTC, 0x28 );
  out8(0x70, 0x0b);
  uint8_t prev = in8(0x71);
  out8(0x70, 0x0b);
  // setting bit 6 enables the RTC with periodic firing (at base rate)
  out8(0x71, prev | 0x40);
  // read RTC -> needed to get things going
  out8(0x70,0x0C);
  in8(0x71);
}
