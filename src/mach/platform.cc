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
#include "mach/platform.h"
#include "mach/asm_functions.h"

extern void Breakpoint(vaddr ia) {
  asm volatile( "xchg %%bx, %%bx" ::: "memory" );
}

void Reboot(vaddr ia) {
  Breakpoint(ia);
  asm volatile("cli" ::: "memory");       // disable interrupts
  while (in8(0x64) & 0x01) in8(0x60);     // clear read buffer
  while (!(in8(0x64) & 0x01));            // wait for any key press
  loadIDT(0,0);                           // load empty IDT
  asm volatile("int $0xff" ::: "memory"); // trigger triple fault
}
