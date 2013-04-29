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
#ifndef _stack_h_
#define _stack_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

// switch to new stack, ignore current stack
extern "C" void stackStart(mword next);

// use next pointer to make switch work, even if curr == next
extern "C" void stackSwitch(mword* curr, mword* next);

// initialize stack for simple invocation of 'func'
extern "C" mword stackInitSimple(mword stack, funcvoid_t func);

// initialize stack for indirect invocation of 'func(data)' via 'start'
extern "C" mword stackInitIndirect(mword stack, function_t func, ptr_t data, void* start);

#endif /* _stack_h_ */
