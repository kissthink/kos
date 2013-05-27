.text
.align  8

# GdbCpuState::saveRegisters()
.globl _ZN11GdbCpuState13saveRegistersEv
# GdbCpuState::stackPtr()
.globl _ZN11GdbCpuState8stackPtrEv
# GdbCpuState::restoreRegisters()
.globl _ZN11GdbCpuState16restoreRegistersEv

.globl handle_exception

# Restore the program's registers (including stack pointer, which
# means we get the right stack and don't have to worry about popping our
# return address and any stack frames and so on) and return.
# cs, rip, ss, rsp, eflags are restored by iretq
# fs, gs are not modified by GDB and must be set by wrmsr instruction

# requires reg64Buffer to be first member variable (access via $rdi)
_ZN11GdbCpuState13saveRegistersEv:
    mov %rax,    (%rdi)
    mov %rbx,   8(%rdi)
    mov %rcx,  16(%rdi)
    mov %rdx,  24(%rdi)
    pop %rax    # return address ?? (ignore)
    pop %rax    # rsi
    mov %rax,  32(%rdi)
#    mov %rsi,  32(%rdi)
    pop %rax
    mov %rax,  40(%rdi)
#    mov %rdi,  40(%rdi)
    mov %rbp,  48(%rdi)
# save stack ptr at saveRegisters2
#   mov %rsp,  56(%rdi)
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
# saveRegisters2
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
    call _ZN11GdbCpuState8stackPtrEv
    mov %rax, %rsp
    mov %rbx, %rdi
    call handle_exception

_ZN11GdbCpuState16restoreRegistersEv:
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
    mov    (%rdi), %rax

    mov  40(%rdi), %rdi     # restore rdi here
# iretq requires
# ss
# rsp
# eflags
# cs
# rip
    iretq

# catchException
.macro CatchEx_TOP ex_num
.globl catchException\ex_num
.align  8
catchException\ex_num:
.endm

.macro CatchEx ex_num
CatchEx_TOP \ex_num
    push %rdi           # save rdi, rsi
    push %rsi
#    mov currentCpu, %rdi
    mov %fs:48, %rdi
    mov $\ex_num, %rsi
    call _ZN11GdbCpuState13saveRegistersEv
.endm

CatchEx 0
CatchEx 1
CatchEx 2
CatchEx 3
CatchEx 4
CatchEx 5
CatchEx 6
CatchEx 7
CatchEx 9
CatchEx 16
