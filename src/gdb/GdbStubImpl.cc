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
#include "dev/Serial.h"
#include "util/Log.h"
#include "mach/Machine.h"

unsigned int sys_gdb_port;

void putDebugChar(unsigned char ch) {
  SerialDevice0::write(ch);
}

int getDebugChar() {
  return SerialDevice0::read();
}

void exceptionHandler(int exceptionNumber, void (*exceptionAddr)()) {
  Machine::setupIDT(exceptionNumber, reinterpret_cast<vaddr>(exceptionAddr));
}
