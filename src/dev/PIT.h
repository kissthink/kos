/******************************************************************************
    Copyright 2013 Behrooz Shafiee, Martin Karsten

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
#ifndef _PIT_h_
#define _PIT_h_ 1

#include "mach/platform.h"

extern "C" void callout_tick(); // from FreeBSD code

class PIT {
  static const int frequency = 1000;
  volatile mword currentTick;
public:
  PIT() : currentTick(0) {}
  void init() volatile                                 __section(".boot.text");
  void staticInterruptHandler() volatile {
    currentTick += 1;
//    if (currentTick % 16 == 0) callout_tick(); // callout_tick for every tick seems too fast...(XXX is this too slow?)
    callout_tick();
  }
  void wait(mword miliseconds) volatile {
    mword start = currentTick;
    while (currentTick < start + miliseconds) Pause();
  }
  mword tick() volatile {
    return currentTick;
  }
};

#endif /* _PIT_h_ */
