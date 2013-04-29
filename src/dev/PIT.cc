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
#include "mach/APIC.h"
#include "mach/Machine.h"
#include "dev/PIT.h"

void PIT::init() { // http://www.jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html
	// Firstly, register our timer callback.
	Machine::staticEnableIRQ(PIC::PIT, 0x20);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	uint32_t divisor = 1193180 / frequency;

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	uint8_t low = (uint8_t)(divisor & 0xFF);
	uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

	// Send the command byte.
	out8(0x43, 0x36);

	// Send the frequency divisor.
	out8(0x40, low);
	out8(0x40, high);
}
