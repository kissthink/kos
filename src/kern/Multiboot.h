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
#ifndef _Multiboot_h_
#define _Multiboot_h_ 1

#include "mach/platform.h"

class FrameManager;

class Multiboot {
  vaddr mbiStart;
  vaddr mbiEnd;
public:
  vaddr init(mword magic, vaddr mbi)               __section(".boot.text");
  void initDebug(bool msg)                         __section(".boot.text");
  void parseAll(laddr& modStart, laddr& modEnd)    __section(".boot.text");
  vaddr getRSDP()                                  __section(".boot.text");
  void getMemory(FrameManager& fm)                 __section(".boot.text");
  void getModules(FrameManager& fm, size_t align)  __section(".boot.text");
  void readModules(vaddr, FrameManager&, size_t)   __section(".boot.text");
};

#endif /* _Multiboot_h_ */
