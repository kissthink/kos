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
#ifndef _asm_functions_h_
#define _asm_functions_h_ 1

#include "util/basics.h"

extern "C" void loadIDT(ptr_t idt, uint16_t size);
extern "C" void loadGDT(ptr_t gdt, uint16_t size);
extern "C" void clearLDT();
extern "C" void loadTR(uint16_t selector);

#endif /* _asm_functions_h_ */
