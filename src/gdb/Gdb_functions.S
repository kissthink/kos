.text
.align  8

# GdbCpuState::restoreRegisters()
.globl _ZN3gdb11GdbCpuState16restoreRegistersEv
# GdbCpuState::saveRegisters1()
.globl _ZN3gdb11GdbCpuState14saveRegisters1Ev
# GdbCpuState::saveRegisters2()
.globl _ZN3gdb11GdbCpuState14saveRegisters2Ev
# GdbCpuState::saveErrorCode()
.globl _ZN3gdb11GdbCpuState13saveErrorCodeEv
# _remcomHandler
.globl _remcomHandler
# mem_fault
.global mem_fault

_ZN3gdb11GdbCpuState16restoreRegistersEv:
#    mov (%rdi),     %rax
    mov 8(%rdi),    %rbx
    mov 16(%rdi),   %rcx
    mov 24(%rdi),   %rdx
    mov 32(%rdi),   %rsi
    # restore rdi later
    mov 48(%rdi),   %rbp

    mov 64(%rdi),   %r8
    mov 72(%rdi),   %r9
    mov 80(%rdi),   %r10
    mov 88(%rdi),   %r11
    mov 96(%rdi),   %r12
    mov 104(%rdi),  %r13
    mov 112(%rdi),  %r14
    mov 120(%rdi),  %r15

    mov 148(%rdi),  %ds
    mov 152(%rdi),  %es

    mov 144(%rdi),  %rax    # push ss
    push %rax
    mov 56(%rdi),   %rax    # push rsp
    push %rax
    mov 136(%rdi),  %eax    # push eflags
    push %rax
    mov 140(%rdi),  %eax    # push cs
    push %rax
    mov 128(%rdi),  %rax    # push rip
    push %rax

    mov (%rdi),     %rax    # restore rax
    mov 40(%rdi),   %rdi    # restore rdi

    iretq

_ZN3gdb11GdbCpuState14saveRegisters1Ev:
    mov %rax,   (%rdi)
    mov %rbx,   8(%rdi)
    mov %rcx,   16(%rdi)
    mov %rdx,   24(%rdi)
    mov %rsi,   32(%rdi)
    mov %rdi,   40(%rdi)
    mov %rbp,   48(%rdi)
    mov %r8,    64(%rdi)
    mov %r9,    72(%rdi)
    mov %r10,   80(%rdi)
    mov %r11,   88(%rdi)
    mov %r12,   96(%rdi)
    mov %r13,   104(%rdi)
    mov %r14,   112(%rdi)
    mov %r15,   120(%rdi)
    mov %ds,    148(%rdi)
    mov %es,    152(%rdi)
    mov %fs,    156(%rdi)
    mov %gs,    160(%rdi)

    ret

_ZN3gdb11GdbCpuState14saveRegisters2Ev:
    pop %rbx
    mov %rbx, %rax  # save return address
    pop %rbx
    pop %rbx
    pop %rbx
    pop %rbx        # actual registers we interested start here
    pop %rbx
    mov %rbx,   128(%rdi)       # save RIP
    pop %rbx
    movl %ebx,  140(%rdi)       # save CS
    pop %rbx
    movl %ebx,  136(%rdi)       # save EFLAGS
    pop %rbx
    mov %rbx,   56(%rdi)        # save RSP
    pop %rbx
    movl %ebx,  144(%rdi)       # save SS
    push %rax       # restore return address

    ret

_ZN3gdb11GdbCpuState13saveErrorCodeEv:
    pop %rbx
    mov %rbx,  164(%rdi) 

    ret

_remcomHandler:
    pop %rax        # pop off return address
    pop %rax        # get exception number
    mov %rax, %rbx  # save rax
    call _ZN3gdb11GdbCpuState8stackPtrEv    # return value at rax
    mov %rax, %rsp  # move to the GdbCpuState::stackPtr
    mov %rbx, %rdi  # pass exception as argument
    call handle_exception

mem_fault:
    pop %rax                # OK to clobber temp registers; we're just going to end up in
    mov %rax, 164(%rdi)   # set_mem_error. pop error code from the stack and save it.
    pop %rax                # rip

    # we don't want to return there, we want to return to the function
    # pointed to by mem_fault_routine instead.
    mov mem_fault_routine, %rax
    pop %rcx                # cs (low 16 bits; junk in high 16 bits)
    pop %rdx                # eflags

    # remove this stack frame; when we do the iret, we will be going to the start
    # of a function, so we want the stack to look just like it
    # would after a "call" instruction.
    leave
    
    # push the stuff that iret wants
    push %rdx               # eflags
    push %rcx               # cs
    push %rax               # eip

    # zero mem_fault_routine
    mov $0, %rax
    mov %rax, mem_fault_routine

    iret



# catchException
.macro CatchEx_TOP ex_num
.globl catchException\ex_num
.align 8
catchException\ex_num:
.endm

.macro CatchEx_BOTTOM ex_num
    call _remcomHandler
.endm

.macro CheckFault ex_num
    cmpl $0, mem_fault_routine
    jne mem_fault
.endm

.macro CatchEx ex_num
CatchEx_TOP \ex_num
    mov currentCpu, %rdi
//    push %rdi       # push GdbCpuState object ptr
    call _ZN3gdb11GdbCpuState13saveRegistersEv
    push $\ex_num
CatchEx_BOTTOM \ex_num
.endm

.macro CatchExError ex_num
CatchEx_TOP \ex_num
    mov currentCpu, %rdi
//    push %rdi       # push GdbCpuState object ptr
    call _ZN3gdb11GdbCpuState26saveRegistersWithErrorCodeEv
    push $\ex_num
CatchEx_BOTTOM \ex_num
.endm

.macro CatchExErrorCheck ex_num
CatchEx_TOP \ex_num
    CheckFault \ex_num
    mov currentCpu, %rdi
#    push %rdi
    call _ZN3gdb11GdbCpuState26saveRegistersWithErrorCodeEv
    push $\ex_num
CatchEx_BOTTOM \ex_num
.endm

CatchEx 0
CatchEx 1
CatchEx 2
CatchEx 3
CatchEx 4
CatchEx 5
CatchEx 6
CatchEx 7
CatchExError 8
CatchEx 9
CatchExError 10
CatchExErrorCheck 11
CatchExError 12
CatchExErrorCheck 13
CatchExErrorCheck 14
CatchEx 16
