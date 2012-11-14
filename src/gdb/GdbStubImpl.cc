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
#include "dev/IODev.h"
#include "util/Log.h"
#include "mach/Machine.h"

unsigned int sys_gdb_port;
static SerialDevice serialDevice;       // InitSerialPort done while creating SerialDevice

extern void set_debug_traps();          // from x86_64-stub.cc
extern void breakpoint();               // from x86_64-stub.cc

void putDebugChar(unsigned char ch) {
    streamsize written = serialDevice.write((const char *)&ch, 1);
    KASSERT(written == 1, "");
}

int getDebugChar() {
    char ch;
    streamsize read = serialDevice.read(&ch, 1);
    KASSERT(read == 1, "");
    return ch;
}

void exceptionHandler(int exceptionNumber, void (*exceptionAddr)()) {
    Machine::setupIDT(exceptionNumber, reinterpret_cast<vaddr>(exceptionAddr));
}

void startGdb() {
    set_debug_traps();
    kcout << "Waiting for GDB(" << sys_gdb_port << ") : " << kendl;
    breakpoint();
//    kcout << "Connected!" << kendl;
}
