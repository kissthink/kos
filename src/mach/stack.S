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
# store/restore caller-owned registers
# see http://x86-64.org/documentation/abi.pdf, Sec 3.2.1

.macro STACKSAVE
	push %rbp
	push %rbx
	push %r12
	push %r13
	push %r14
	push %r15
.endm

.macro STACKRESTORE
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbx
	pop %rbp
.endm

.globl	stackSwitch
.align	8
stackSwitch:       # (curr*, next*)
	STACKSAVE
	mov %rsp, (%rdi) # save current stack
	mov (%rsi), %rsp # load next stack
	STACKRESTORE
	ret

.globl	stackSwitchAS
.align	8
stackSwitchAS:     # (curr*, next*, nextCR3)
	STACKSAVE
	mov %rsp, (%rdi) # save current stack
	mov %rdx, %cr3   # set new address space
	mov (%rsi), %rsp # load next stack
	STACKRESTORE
	ret

.globl	stackStart
.align	8
stackStart:      # (next)
mov %rdi, %rsp   # load next stack
	STACKRESTORE
	ret

# stub function needed to properly pass 'func' and 'data' to 'start'
.globl stackInitIndirect
.align 8
stackInitIndirect:            # stack, func, data, start -> new stack
	movq %rsi, -64(%rdi)        # store 'func' for stub function (via r15)
	movq %rdx, -56(%rdi)        # store 'data' for stub function (via r14)
	movq %rcx, -48(%rdi)				# store 'start' for stub function (via r13)
#	movq $0, -40(%rdi)          # indirectly set r12 to 0
#	movq $0, -32(%rdi)          # indirectly set rbx to 0
	movq $0, -24(%rdi)          # indirectly set rbp to 0
	movq $stackStub, -16(%rdi)  # push stub function as return address
	leaq -64(%rdi), %rax        # prepare return value
	ret

.align 8
stackStub:
	movq %r15, %rdi							# move 'func' into first argument
	movq %r14, %rsi             # move 'data' into second argument
	jmp  *%r13                  # jmp to start function

.globl stackInitSimple
.align 8
stackInitSimple:              # stack, func -> new stack
#	movq $0, -64(%rdi)          # indirectly set r15 to 0
#	movq $0, -56(%rdi)          # indirectly set r14 to 0
#	movq $0, -48(%rdi)          # indirectly set r13 to 0
#	movq $0, -40(%rdi)          # indirectly set r12 to 0
#	movq $0, -32(%rdi)          # indirectly set rbx to 0
	movq $0, -24(%rdi)          # indirectly set rbp to 0
	movq %rsi, -16(%rdi)        # push 'func' function as return address
	leaq -64(%rdi), %rax        # prepare return value
	ret
