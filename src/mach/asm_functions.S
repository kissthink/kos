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
.text
.align   8
.globl loadIDT
loadIDT:
	mov %rdi, IDT64P
	sub $1, %rsi
	mov %si, IDT64
	lidt IDT64
	ret
IDT64:
.short 0 # size - 1
IDT64P:
.quad 0  # pointer to table

.align   8
.globl loadGDT
loadGDT:
	mov %rdi, GDT64P
	sub $1, %rsi
	mov %si, GDT64
	lgdt GDT64
	ret
GDT64:
.short 0 # size - 1
GDT64P:
.quad 0  # pointer to table

# Entries in the GDT can point to an LDT, which is used when the TI bit in
# the segment selector is set to 1.  The LDTR base register is loaded with
# the GDT entry. Here the LDTR is loaded with GDT's null selector to
# invalidate the LDT.
.align   8
.globl clearLDT
clearLDT:
	xor %ax, %ax
	lldt %ax
	ret

# The task register (TR) points to a task selector in the GDT, which in
# turns point to the task-state segment (TSS).  The TSS is used for
# inner-level stack pointers and the IO bit-map. Others?
.align 8
.globl loadTR
loadTR:
	ltr %di
	ret

.align   8
.globl   syscallHandler
syscallHandler:
	pushq %rcx
	call handleSysCall
	popq %rcx
	sysretq
