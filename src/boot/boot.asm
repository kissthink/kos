;
;   Copyright 2012 Martin Karsten
;
;   This program is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
; adapted from http://wiki.osdev.org/64-bit_Higher_Half_Kernel_with_GRUB_2

[BITS 32]
[SECTION .mbhdr]
[EXTERN _loadStart]
[EXTERN _loadEnd]
[EXTERN _bssEnd]

ALIGN 8
mbHdr:
	DD	0xE85250D6                                              ; Magic
	DD	0                                                       ; Architecture
	DD	mbHdrEnd - mbHdr                                        ; Length
	DD	(0xFFFFFFFF-(0xE85250D6 + 0 + (mbHdrEnd - mbHdr))) + 1  ; Checksum
 
mbTagInfo:      ; Request information from boot loader - all possible tags
	DW	1, 0
	DD	(mbTagInfoEnd - mbTagInfo)
  DD  1         ; boot command line
  DD  2         ; boot loader name
  DD  3         ; loaded modules
  DD  4         ; basic memory information
  DD  5         ; BIOS boot device
  DD  6         ; detailed memory map
  DD  7         ; VBE info
  DD  8         ; framebuffer info
  DD  9         ; ELF sections
  DD  10        ; APM table
  DD  11        ; EFI32
  DD  12        ; EFI64
;  DD  13        ; SMBIOS - not currently supported by grub2
  DD  14        ; ACPI (old)
  DD  15        ; ACPI (new)
;  DD  16        ; network - not currently supported by grub2
mbTagInfoEnd:

; not handled by grub2 - not currently needed anyway
;	DW	2, 0      ; Sections override 
;	DD	24
;	DD	mbHdr
;	DD	_loadStart
;	DD	_loadEnd
;	DD	_bssEnd
 
;	DW	3, 0      ; Entry point override
;	DD	12
;	DD	entryPoint
 
	DD	0, 0      ; End Of Tags
	DW	8
mbHdrEnd:

[SECTION .boot]
[BITS 16]              ; entry point for Startup IPI starting AP
[GLOBAL boot16Begin]   ; BSP copies this code to BOOT16
boot16Begin:
	cli                  ; disable interrupts
	in al, 0x92          ; fast enable A20, see http://wiki.osdev.org/A20_Line
	or al, 2
	out 0x92, al
	jmp 0:BOOT16 + clearcs - boot16Begin
clearcs:
	lgdt [cs:BOOT16 + Gdtr16 - boot16Begin]
	mov eax, cr0         ; enter protected mode
	or al, 1
	mov cr0, eax
	jmp 8:BOOT16 + boot16Trampoline - boot16Begin
Gdtr16:                ; temporary gdt
	DW	23
	DD	BOOT16 + TmpGdt16 - boot16Begin
TmpGdt16:
	DQ	0x0000000000000000
	DQ	0x00CF9A000000FFFF
	DQ	0x00CF92000000FFFF
[BITS 32]
boot16Trampoline:
	mov eax, 0x10        ; set up segment registers
	mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
	mov esp, BOOT16 + boot16End - boot16Begin   ; set up first stack
	xor eax, eax         ; set fake multiboot arguments to indicate AP
	mov ebx, 0xE85250D6
	mov ecx, entryPoint  ; enter regular bootstrap code path
	jmp ecx           
ALIGN 4096
[GLOBAL boot16End]
boot16End:

[BITS 32]              ; entry point for the multiboot loader starting the BSP
[GLOBAL entryPoint]
entryPoint:
	push 0x0             ; store multiboot arguments for kmain
	push eax
	push 0x0
	push ebx

	mov eax, 0x80000000  ; test for extended function 0x80000000
	cpuid
	cmp eax, 0x80000000
	jle NoLongMode
	mov eax, 0x80000001  ; test for 64-bit mode support
	cpuid
	and edx, (1<<29)
	jz NoLongMode

	lgdt [Gdtr1]         ; create temporary 32-bit GDT
	jmp 8:GdtReady

NoLongMode:
	mov esi, ErrorMsg64  ; print error message
	cld
NextChar:
	lodsb
	cmp al, 0
	je Reboot
	mov edi, [Cursor]
	stosb
	mov byte [edi], 0x07
	add dword [Cursor], 2
	jmp NextChar

Reboot:                ; reboot after key press
	
KeyboardDrain:         ; clear keyboard buffer
  in al, 0x64
  test al, 0x01
  jz KeyboardWait
  in al, 0x60
  jmp KeyboardDrain

KeyboardWait:          ; wait for keypress and reboot by triple fault
  in al, 0x64
  test al, 0x01
  jz KeyboardWait
  int 0xff             ; trigger triple fault
  jmp $

ErrorMsg64:
	db 'ERROR: No 64-Bit Mode. Press Key to Reboot.'
	db 0

Cursor:
	dd 0xB8000

GdtReady:
	call SetupPaging
	call SetupLongMode
 
	lgdt [Gdtr2]         ; create new 64-bit GDT
	jmp 8:Gdt2Ready
 
[BITS 64]
[EXTERN kmain]
Gdt2Ready:
	mov eax, 0x10        ; set up segment registers
	mov ds, ax
	mov es, ax
	mov ss, ax

	pop rsi              ; restore multiboot arguments and set up stack
	pop rdi
	add rsi, KERNBASE
	mov rsp, KERNBASE + Stack
	push 0x0             ; push 0x0 to tell gdb end of stack frame

	jmp kmain            ; enter OS kernel

[BITS 32]
SetupPaging:
	mov eax, Pdpt        ; set up page tables
	or eax, 1
	mov [Pml4], eax      ; use bits 47-39 for pml4
	mov [Pml4 + ((KERNBASE % 0x1000000000000) / 0x8000000000) * 8], eax
 
	mov eax, Pd
	or eax, 1
	mov [Pdpt], eax      ; use bits 38-30 for pdpt
	mov [Pdpt + ((KERNBASE % 0x8000000000) / 0x40000000) * 8], eax

	mov ecx, BOOTMEM
	mov edx, Pd
	mov eax, 0x000083 
mapInitPages:
	mov dword [edx], eax
	add edx, 8
	add eax, 0x200000
	sub ecx, 0x200000
	jnz mapInitPages
	ret
 
SetupLongMode:
	mov eax, Pml4        ; load CR3 with PML4
	mov cr3, eax
	mov eax, cr4         ; enable PAE
	or eax, 1 << 5
	mov cr4, eax
	mov ecx, 0xC0000080  ; enable Long Mode in the MSR
	rdmsr
	or eax, 1 << 8
	wrmsr
	mov eax, cr0         ; enable paging
	or eax, 1 << 31
	mov cr0, eax
	ret

Gdtr1:
	DW	23
	DD	TmpGdt1
TmpGdt1:
	DQ	0x0000000000000000
	DQ	0x00CF9A000000FFFF
	DQ	0x00CF92000000FFFF

Gdtr2:
	DW	23
	DD	TmpGdt2
TmpGdt2:
	DQ	0x0000000000000000
	DQ	0x00209A0000000000
	DQ	0x0020920000000000

ALIGN 4096
Pml4:
	TIMES 0x1000 DB 0
Pdpt:
	TIMES 0x1000 DB 0
Pd:
	TIMES 0x1000 DB 0
	TIMES 0x4000 DB 0
Stack:
