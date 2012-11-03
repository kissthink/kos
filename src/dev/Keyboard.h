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
/* Simple PS/2 keyboard driver */

#ifndef _Keyboard_h_
#define _Keyboard_h_ 1

#include "dev/Device.h"
#include "kern/PCQ.h"

class Keyboard : public Device {
  PCQ<uint16_t>& kcq;
public:
  explicit Keyboard(PCQ<uint16_t>& k) : kcq(k) { initAtBoot(); }
  virtual bool init();            __section(".kernel.init");
  void staticInterruptHandler();
};

#endif /* _Keyboard_h_ */
