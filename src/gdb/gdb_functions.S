.text
.global return_to_prog
return_to_prog:
    movq _registers+8,  %rbx
    movq _registers+16, %rcx
    movq _registers+24, %rdx
    movq _registers+32, %rsi
    movq _registers+40, %rdi
    movq _registers+48, %rbp
# stack pointer pushed to stack for 64 bit IRETQ
    movq _registers+64, %r8
    movq _registers+72, %r9
    movq _registers+80, %r10
    movq _registers+88, $r11
    movq _registers+96, %r12
    movq _registers+104,%r13
    movq _registers+112,%r14
    movq _registers+120,%r15
    movw _ds,   %ds
    movw _es,   %es
    movw _fs,   %fs
    movw _gs,   %gs
    xor %rax, %rax
    iretq

.global save_registers1
save_registers1:
    movq %rax,  _registers
    movq %rbx,  _registers+8
    movq %rcx,  _registers+16
    movq %rdx,  _registers+24
    movq %rsi,  _registers+32
    movq %rdi,  _registers+40
    movq %rbp,  _registers+48
    movq %r8,   _registers+56
    movq %r9,   _registers+64
    movq %r10,  _registers+72
    movq %r11,  _registers+80
    movq %r12,  _registers+88
    movq %r13,  _registers+96
    movq %r14,  _registers+104
    movq %r15,  _registers+112
# segment registers CS, SS are pushed to stack by INT
    movw %ds,   _ds
    movw %es,   _es
    movw %fs,   _fs
    movw %gs,   _gs

.global save_errorcode
save_errorcode:
    popq %rbx
    movq %rbx, _gdb_x8664errcode

.global 
