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
#ifndef _RTC_h_
#define _RTC_h_ 1

#include "mach/platform.h"
#include "dev/Device.h"

class RTC : public Device {
  mword currentTick;
public:
  RTC() { initAtBoot(); }
  virtual bool init();            __section(".kernel.init");
  void staticInterruptHandler() {
    out8(0x70,0x0C);
    in8(0x71);
    currentTick += 1;
  }
  void wait(mword ticks) {
    mword start = currentTick;
    while (currentTick < start + ticks) Pause();
  }
  mword tick() {
    return currentTick;
  }
};

#endif /* _RTC_h_ */
