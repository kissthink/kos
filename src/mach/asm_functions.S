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
loadIDT:
.globl loadIDT
.align   8
	mov %rdi, IDT64P
	sub $1, %rsi
	mov %si, IDT64
	lidt IDT64
	ret
IDT64:
.short 0 // size - 1
IDT64P:
.quad 0  // pointer to table

loadGDT:
.globl loadGDT
.align   8
	mov %rdi, GDT64P
	sub $1, %rsi
	mov %si, GDT64
	lgdt GDT64
	ret
GDT64:
.short 0 // size - 1
GDT64P:
.quad 0  // pointer to table
