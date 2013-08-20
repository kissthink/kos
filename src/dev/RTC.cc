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
#include "kern/Debug.h"

int daysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void RTC::init() volatile { // see http://wiki.osdev.org/RTC
  Machine::staticEnableIRQ( PIC::RTC, 0x28 );
  // read current time and date http://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
  second = read(0x00);
  minute = read(0x02);
  hour   = read(0x04);
  day    = read(0x07);
  month  = read(0x08);
  year   = read(0x09);
  bool isBCD = (read(0x0b) & 0x04) != 0x04;
  DBG::outln(DBG::Basic, "BCD mode");
  if (isBCD) {  // parse BCD format
    second = ((second & 0xf0)>>1) + ((second & 0xf0)>>3) + (second & 0xf);
    minute = ((minute & 0xf0)>>1) + ((minute & 0xf0)>>3) + (minute & 0xf);
    hour   = ((hour & 0xf0)>>1) + ((hour & 0xf0)>>3) + (hour & 0xf);
    day    = ((day & 0xf0)>>1) + ((day & 0xf0)>>3) + (day & 0xf);
    month  = ((month & 0xf0)>>1) + ((month & 0xf0)>>3) + (month & 0xf);
    year  = ((year & 0xf0)>>1) + ((year & 0xf0)>>3) + (year & 0xf);
  }
  year += 2000; // full 4-digit year
  out8(0x70, 0x0b);
  uint8_t prev = in8(0x71);
  out8(0x70, 0x0b);
  // setting bit 6 enables the RTC with periodic firing (at base rate)
  out8(0x71, prev | 0x40);
  // read RTC -> needed to get things going
  out8(0x70,0x0C);
  in8(0x71);
}

void RTC::updateTime() volatile {
  ticksPerSec += 1;  // assuming base rate 1024HZ
  if (ticksPerSec == 1024) {
    second += 1;
    if (second == 60) {
      second = 0;
      minute += 1;
      if (minute == 60) {
        minute = 0;
        hour += 1;
        if (hour == 24) {
          hour = 0;
          day += 1;
          // http://en.wikipedia.org/wiki/Leap_year
          bool leapYear = ((year % 400) == 0) || (((year % 100) != 0) && ((year % 4) == 0));
          if (day > daysPerMonth[month-1]) {
            if ((month != 2) || !leapYear) {
              month += 1;
              day = 1;
            } else {  // feb and is leap year
              if (day == 29) {
                month += 1;
                day = 1;
              }
            }
          }
          if (month == 12) {
            year += 1;
            month = 1;
          }
        }
      }
    }
    ticksPerSec = 0;
    DBG::outln(DBG::Basic, year, '-', (int)month, '-', (int)day, ' ', (int)hour, ':', (int)minute, ':', (int)second);
  }
}
