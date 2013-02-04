.code32
.section ".mbhdr","a"
.extern _loadStart
.extern _loadEnd
.extern _bssEnd

.align 8
mbHdr:
    .long 0xE85250D6                                                 # Magic
    .long 0                                                          # Architecture
    .long mbHdrEnd - mbHdr                                           # Length
    .long (0xFFFFFFFF-(0xE85250D6 + 0 + (mbHdrEnd - mbHdr))) + 1     # Checksum

mbTagInfo:         # Request information from boot loader - all possible tags
    .short 1, 0
    .long (mbTagInfoEnd - mbTagInfo)
    .long 1        # boot command line
    .long 2        # boot loader name
    .long 3        # loaded modules
    .long 4        # basic memory information
    .long 5        # BIOS boot device
    .long 6        # detailed memory map
    .long 7        # VBE info
    .long 8        # framebuffer info
    .long 9        # ELF sections
    .long 10       # APM table
    .long 11       # EFI32
    .long 12       # EFI64
#    .long 13      # SMBIOS - not currently supported by grub2
    .long 14      # ACPI (old)
    .long 15      # ACPI (new)
#    .long 16      # network - not currently supported by grub2
mbTagInfoEnd:

# not handled by grub2 - not currently needed anyway
#    .short 2, 0      # Sections override 
#    .long 24
#    .long mbHdr
#    .long _loadStart
#    .long _loadEnd
#    .long _bssEnd
 
#    .short 3, 0      # Entry point override
#    .long 12
#    .long entryPoint
 
    .long 0, 0      # End Of Tags
    .short 8            
mbHdrEnd:

.section ".boot","ax"
.code16                     # entry point for Startup IPI starting AP
.global boot16Begin         # BSP copies this code to BOOT16
boot16Begin:
    cli                     # disable interrupts
    in $0x92, %al           # fast enable A20, see http:#wiki.osdev.org/A20_Line
    or $2, %al
    out %al,$0x92
    ljmp $0,$(BOOT16+clearcs-boot16Begin)
clearcs:
    lgdt %cs:(BOOT16 + Gdtr16 - boot16Begin)
    mov %cr0, %eax          # enter protected mode
    or $1, %al
    mov %eax, %cr0
    jmp $0x8,$(BOOT16 + boot16Trampoline - boot16Begin)
Gdtr16:                     # temporary gdt
    .short 23
    .long  BOOT16 + TmpGdt16 - boot16Begin
TmpGdt16:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
.code32
boot16Trampoline:
    mov $0x10, %eax         # set up segment registers
    mov %ax, %ds  
    mov %ax, %es 
    mov %ax, %fs
    mov %ax, %gs 
    mov %ax, %ss 
    mov $(BOOT16 + boot16End - boot16Begin), %esp # set up first stack
    xor %eax, %eax          # set fake multiboot arguments to indicate AP
    mov $0xE85250D6,%ebx 
    mov $entryPoint ,%ecx   # enter regular bootstrap code path
    jmp *%ecx           
.align 0x1000
.global boot16End
boot16End:

.code32
.global entryPoint
entryPoint:
    push $0x0               # store multiboot arguments for kmain
    push %eax
    push $0x0
    push %ebx

    mov $0x80000000,%eax    # test for extended function 0x80000000
    cpuid
    cmp $0x80000000, %eax
    jle NoLongMode
    mov $0x80000001, %eax   # test for 64-bit mode support
    cpuid
    and $(1<<29), %edx
    jz NoLongMode

    lgdt (Gdtr1)            # create temporary 32-bit GDT
    jmp $0x8,$GdtReady

NoLongMode:
    mov $ErrorMsg64, %esi   # print error message
    cld
NextChar:
    lodsb
    cmp $0, %al
    je Reboot
    mov (Cursor),%edi 
    stosb
    movb $0x07, (%edi)
    addl $2, (Cursor)
    jmp NextChar

Reboot:                     # reboot after key press
    
KeyboardDrain:              # clear keyboard buffer
    in $0x64, %al
    test $0x01, %al
    jz KeyboardWait
    in $0x60,%al 
    jmp KeyboardDrain

KeyboardWait:               # wait for keypress and reboot by triple fault
    in $0x64, %al 
    test $0x01, %al 
    jz KeyboardWait
    int $0xff               # trigger triple fault
    jmp .

ErrorMsg64:
    .ascii "ERROR: No 64-Bit Mode. Press Key to Reboot."
    .byte 0

Cursor:
    .long 0xB8000

GdtReady:
    call SetupPaging
    call SetupLongMode
 
    lgdt (Gdtr2)            # create new 64-bit GDT
    jmp $8,$Gdt2Ready
 
.code64
.extern kmain
Gdt2Ready:
    mov $0x10,%eax          # set up segment registers
    mov %ax, %ds 
    mov %ax, %es 
    mov %ax, %ss 

    pop %rsi                # restore multiboot arguments and set up stack
    pop %rdi
    add $KERNBASE, %rsi
    mov $KERNBASE + Stack, %rsp
    push $0x0               # push 0x0 to tell gdb end of stack frame

    jmp kmain               # enter OS kernel

.code32
SetupPaging:
    xchg %bx, %bx
    mov $Pdpt, %eax         # set up page tables
    or $1, %eax 
    mov %eax, (Pml4)        # use bits 47-39 for pml4, 8 bytes per entry
    mov %eax, (Pml4 + (((KERNBASE >> 39) % (1<<9))<<3))
 
    mov $Pd, %eax 
    or $1, %eax 
    mov %eax, (Pdpt)        # use bits 38-30 for pdpt, 8 bytes per entry
    mov %eax, (Pdpt + (((KERNBASE >> 30) % (1<<9))<<3))

    mov $BOOTMEM, %ecx 
    mov $Pd, %edx 
    mov $0x000083, %eax 
mapInitPages:
    movl %eax, (%edx) 
    add $8 , %edx 
    add $0x200000, %eax 
    sub $0x200000, %ecx 
    jnz mapInitPages
    ret
 
SetupLongMode:
    mov $Pml4, %eax         # load CR3 with PML4
    mov %eax, %cr3 
    mov %cr4, %eax          # enable PAE
    or $(1 << 5), %eax 
    mov %eax, %cr4 
    mov $0xC0000080, %ecx   # enable Long Mode in the MSR
    rdmsr
    or $(1 << 8), %eax 
    wrmsr
    mov %cr0, %eax          # enable paging
    or $(1 << 31), %eax 
    mov %eax, %cr0 
    ret

Gdtr1:
    .short 23
    .long TmpGdt1
TmpGdt1:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF

Gdtr2:
    .short 23
    .long TmpGdt2
TmpGdt2:
    .quad 0x0000000000000000
    .quad 0x00209A0000000000
    .quad 0x0020920000000000

.align 0x1000
Pml4:
    .rept 0x1000
    .byte 0
    .endr
Pdpt:
    .rept 0x1000
    .byte 0
    .endr
Pd:
    .rept 0x1000
    .byte 0
    .endr
    .rept 0x4000
    .byte 0
    .endr
Stack:

.section ".text"
.section ".data"
.section ".bss"
