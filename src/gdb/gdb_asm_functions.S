.text

# Restore the program's registers (including stack pointer, which
# means we get the right stack and don't have to worry about popping our
# return address and any stack frames and so on) and return.
# cs, rip, ss, rsp, eflags are restored by iretq
# fs, gs are not modified by GDB and must be set by wrmsr instruction

# requires reg64Buffer to be at %rdi
.align  8
saveRegisters:
    mov %rax,    (%rdi)
    mov %rbx,   8(%rdi)
    mov %rcx,  16(%rdi)
    mov %rdx,  24(%rdi)
    pop %rax    # return address ?? (ignore)
    pop %rax    # rsi
    mov %rax,  32(%rdi)
    pop %rax    # rdi
    mov %rax,  40(%rdi)
    mov %rbp,  48(%rdi)
#   mov %rsp,  56(%rdi)    save rsp later
    mov %r8,   64(%rdi)
    mov %r9,   72(%rdi)
    mov %r10,  80(%rdi)
    mov %r11,  88(%rdi)
    mov %r12,  96(%rdi)
    mov %r13, 104(%rdi)
    mov %r14, 112(%rdi)
    mov %r15, 120(%rdi)
    mov %ds,  148(%rdi)
    mov %es,  152(%rdi)
    mov %fs,  156(%rdi)
    mov %gs,  160(%rdi)
#    pop %rbx    # return address for catchException <- want this?
    pop %rbx    # return address to breakpoint()    <- want this
    mov %rbx,  128(%rdi)     # rip
    pop %rbx
    movl %ebx, 140(%rdi)     # cs
    pop %rbx
    movl %ebx, 136(%rdi)     # eflags
    pop %rbx
    mov %rbx,   56(%rdi)     # rsp
    pop %rbx
    movl %ebx, 144(%rdi)     # ss
    mov %rsi, %rbx           # exception number

# _remcomHandler
    call getCurrentGdbStack
    mov %rax, %rsp
    mov %rbx, %rdi
    call handle_exception

# requires reg64Buffer to be at %rdi
.globl restoreRegisters
.align  8
restoreRegisters:
#    mov  0(%rdi), %rax    restore rax later
    mov   8(%rdi), %rbx
    mov  16(%rdi), %rcx
    mov  24(%rdi), %rdx
    mov  32(%rdi), %rsi
#    mov  40(%rdi), %rdi    restore rdi later
    mov  48(%rdi), %rbp
    mov  64(%rdi), %r8
    mov  72(%rdi), %r9
    mov  80(%rdi), %r10
    mov  88(%rdi), %r11
    mov  96(%rdi), %r12
    mov 104(%rdi), %r13
    mov 112(%rdi), %r14
    mov 120(%rdi), %r15
    mov 148(%rdi), %ds
    mov 152(%rdi), %es
    xor %rax, %rax
    movl 144(%rdi), %eax    # ss
    push %rax
    xor %rax, %rax
    mov  56(%rdi), %rax     # rsp
    push %rax
    xor %rax, %rax
    movl 136(%rdi), %eax    # eflags
    push %rax
    xor %rax, %rax
    movl 140(%rdi), %eax    # cs
    push %rax
    xor %rax, %rax
    mov 128(%rdi), %rax     # rip
    push %rax

    mov    (%rdi), %rax     # restore rax here
    mov  40(%rdi), %rdi     # restore rdi here
# iretq requires
# ss
# rsp
# eflags
# cs
# rip
    iretq

# catchException
.macro CatchEx ex_num
.globl catchException\ex_num
.align  8
catchException\ex_num:
    push %rdi           # save rdi, rsi
    push %rsi
		push %rax
		call getCurrentGdbCpu
		mov %rax, %rdi
		pop %rax
    mov $\ex_num, %rsi
    call saveRegisters
.endm

.align 8
mem_fault:
    pop %rax                # error code
    pop %rax                # rip (but we jump to mem_fault_routine)
    pop %rax                # cs
    pop %rbx                # eflags
    pop %rcx                # rsp
    pop %rdx                # ss
    push %rdx               # ss
    push %rcx               # rsp
    push %rbx               # eflags
    push %rax               # cs
    mov $1, %rax
    mov %rax, mem_err
    mov mem_fault_return_addr, %rax
    push %rax               # rip
    mov $0, %rax
    mov %rax, mem_fault_routine
    iretq

# catchExceptionFault
.macro CatchExFault ex_num
.globl catchExceptionFault\ex_num
.align 8
catchExceptionFault\ex_num:
    cmp $0, mem_fault_routine
    jne mem_fault
    jmp isr_wrapper_\ex_num
.endm

CatchEx 0x00
CatchEx 0x01
CatchEx 0x02
CatchEx 0x03
CatchEx 0x04
CatchEx 0x05
CatchEx 0x06
CatchEx 0x07
CatchEx 0x09
CatchExFault 0x0c
CatchExFault 0x0d
CatchExFault 0x0e
CatchEx 0x10
