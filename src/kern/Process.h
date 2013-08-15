/******************************************************************************
    Copyright 2013 Martin Karsten

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
#ifndef _Process_h_
#define _Process_h_ 1

#include "kern/AddressSpace.h"

class Process {
  AddressSpace as;
  Thread* thread;
public:
  Process() : as(AddressSpace::User) {
    as.setMemoryRange(userBase, topuser - userBase);
  }
  void execElfFile(const kstring& fileName);
};

#endif /* _Process_h_ */
