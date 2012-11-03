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
# store/restore caller-owned registers
# see http://x86-64.org/documentation/abi.pdf, Sec 3.2.1

.globl	stackSwitch
.align	8
stackSwitch:     # (curr*, next*)
push %rbp
push %rbx
push %r12
push %r13
push %r14
push %r15
mov %rsp, (%rdi) # save current stack
mov (%rsi), %rsp # load next stack
pop %r15
pop %r14
pop %r13
pop %r12
pop %rbx
pop %rbp
ret

.globl	stackStart
.align	8
stackStart:      # (next)
mov %rdi, %rsp   # load next stack
pop %r15
pop %r14
pop %r13
pop %r12
pop %rbx
pop %rbp
ret

# stub function needed to properly pass 'func' and 'data' to 'start'
.globl stackInitIndirect
.align 8
stackInitIndirect:          # stack, func, data, start -> new stack
movq %rsi, -64(%rdi)        # store 'func' for stub function
movq %rdx, -56(%rdi)        # store 'data' for stub function
movq %rcx, -48(%rdi)				# store 'start' for stub function
leaq -64(%rdi), %rax        # prepare return value
movq $stackStub, -16(%rdi)  # push stub function as return address
ret

.align 8
stackStub:
movq %r15, %rdi							# move 'func' into first argument
movq %r14, %rsi             # move 'data' into second argument
jmp  *%r13                  # jmp to start function

.globl stackInitSimple
.align 8
stackInitSimple:            # stack, func -> new stack
leaq -64(%rdi), %rax        # prepare return value
movq %rsi, -16(%rdi)  # push stub function as return address
ret
