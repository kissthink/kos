	.file	"i386-stub.cc"
	.local	_ZL11initialized
	.comm	_ZL11initialized,1,1
	.globl	remote_debug
	.bss
	.align 4
	.type	remote_debug, @object
	.size	remote_debug, 4
remote_debug:
	.zero	4
	.globl	registers
	.align 32
	.type	registers, @object
	.size	registers, 64
registers:
	.zero	64
	.globl	remcomStack
	.align 32
	.type	remcomStack, @object
	.size	remcomStack, 10000
remcomStack:
	.zero	10000
	.globl	stackPtr
	.data
	.align 8
	.type	stackPtr, @object
	.size	stackPtr, 8
stackPtr:
	.quad	remcomStack+9996
#APP
	.text
	.globl return_to_prog
	return_to_prog:
	        movw registers+44, %ss
	        movl registers+16, %esp
	        movl registers+4, %ecx
	        movl registers+8, %edx
	        movl registers+12, %ebx
	        movl registers+20, %ebp
	        movl registers+24, %esi
	        movl registers+28, %edi
	        movw registers+48, %ds
	        movw registers+52, %es
	        movw registers+56, %fs
	        movw registers+60, %gs
	        movl registers+36, %eax
	        pushq %rax
	        movl registers+40, %eax
	        pushq %rax
	        movl registers+32, %eax
	        pushq %rax
	        movl registers, %eax
	        iret
#NO_APP
	.globl	gdb_i386errcode
	.bss
	.align 4
	.type	gdb_i386errcode, @object
	.size	gdb_i386errcode, 4
gdb_i386errcode:
	.zero	4
	.globl	gdb_i386vector
	.data
	.align 4
	.type	gdb_i386vector, @object
	.size	gdb_i386vector, 4
gdb_i386vector:
	.long	-1
#APP
	.text
	mem_fault:
	     popq %rax
	     movl %eax, gdb_i386errcode
	     popq %rax
	     movl mem_fault_routine, %eax
	     popq %rcx
	     popq %rdx
	     leave
	     pushq %rdx
	     pushq %rcx
	     pushq %rax
	     movl $0, %eax
	     movl %eax, mem_fault_routine
	iret
	.text
	.globl catchException3
	catchException3:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $3
	call _remcomHandler
	.text
	.globl catchException1
	catchException1:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $1
	call _remcomHandler
	.text
	.globl catchException0
	catchException0:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $0
	call _remcomHandler
	.text
	.globl catchException4
	catchException4:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $4
	call _remcomHandler
	.text
	.globl catchException5
	catchException5:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $5
	call _remcomHandler
	.text
	.globl catchException6
	catchException6:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $6
	call _remcomHandler
	.text
	.globl catchException7
	catchException7:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $7
	call _remcomHandler
	.text
	.globl catchException8
	catchException8:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, gdb_i386errcode
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $8
	call _remcomHandler
	.text
	.globl catchException9
	catchException9:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $9
	call _remcomHandler
	.text
	.globl catchException10
	catchException10:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, gdb_i386errcode
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $10
	call _remcomHandler
	.text
	.globl catchException12
	catchException12:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, gdb_i386errcode
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $12
	call _remcomHandler
	.text
	.globl catchException16
	catchException16:
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $16
	call _remcomHandler
	.text
	.globl catchException13
	catchException13:
	cmpl $0, mem_fault_routine
	jne mem_fault
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, gdb_i386errcode
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $13
	call _remcomHandler
	.text
	.globl catchException11
	catchException11:
	cmpl $0, mem_fault_routine
	jne mem_fault
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, gdb_i386errcode
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $11
	call _remcomHandler
	.text
	.globl catchException14
	catchException14:
	cmpl $0, mem_fault_routine
	jne mem_fault
	movl %eax, registers
	movl %ecx, registers+4
	movl %edx, registers+8
	movl %ebx, registers+12
	movl %ebp, registers+20
	movl %esi, registers+24
	movl %edi, registers+28
	movw $0, %ax
	movw %ds, registers+48
	movw %ax, registers+50
	movw %es, registers+52
	movw %ax, registers+54
	movw %fs, registers+56
	movw %ax, registers+58
	movw %gs, registers+60
	movw %ax, registers+62
	popq %rbx
	movl %ebx, gdb_i386errcode
	popq %rbx
	movl %ebx, registers+32
	popq %rbx
	movl %ebx, registers+40
	movw %ax, registers+42
	popq %rbx
	movl %ebx, registers+36
	movw %ss, registers+44
	movw %ax, registers+46
	movl %esp, registers+16
	pushq $14
	call _remcomHandler
	_remcomHandler:
	           popq %rax
	           popq %rax
			movl stackPtr, %esp
			pushq %rax
			call  handle_exception
#NO_APP
	.text
	.globl	_Z20_returnFromExceptionv
	.type	_Z20_returnFromExceptionv, @function
_Z20_returnFromExceptionv:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	call	return_to_prog
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	_Z20_returnFromExceptionv, .-_Z20_returnFromExceptionv
	.globl	_Z3hexc
	.type	_Z3hexc, @function
_Z3hexc:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, %eax
	movb	%al, -4(%rbp)
	cmpb	$96, -4(%rbp)
	jle	.L3
	cmpb	$102, -4(%rbp)
	jg	.L3
	movsbl	-4(%rbp), %eax
	subl	$87, %eax
	jmp	.L4
.L3:
	cmpb	$47, -4(%rbp)
	jle	.L5
	cmpb	$57, -4(%rbp)
	jg	.L5
	movsbl	-4(%rbp), %eax
	subl	$48, %eax
	jmp	.L4
.L5:
	cmpb	$64, -4(%rbp)
	jle	.L6
	cmpb	$70, -4(%rbp)
	jg	.L6
	movsbl	-4(%rbp), %eax
	subl	$55, %eax
	jmp	.L4
.L6:
	movl	$-1, %eax
.L4:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	_Z3hexc, .-_Z3hexc
	.local	_ZL14remcomInBuffer
	.comm	_ZL14remcomInBuffer,400,32
	.local	_ZL15remcomOutBuffer
	.comm	_ZL15remcomOutBuffer,400,32
	.section	.rodata
	.align 8
.LC0:
	.string	"bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n"
	.text
	.globl	_Z9getpacketv
	.type	_Z9getpacketv, @function
_Z9getpacketv:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movq	$_ZL14remcomInBuffer, -16(%rbp)
	jmp	.L21
.L24:
	nop
.L20:
.L21:
	nop
.L8:
	call	_Z12getDebugCharv
	movb	%al, -2(%rbp)
	cmpb	$36, -2(%rbp)
	setne	%al
	testb	%al, %al
	jne	.L8
	jmp	.L9
.L22:
	nop
.L9:
	movb	$0, -3(%rbp)
	movb	$-1, -1(%rbp)
	movl	$0, -8(%rbp)
	jmp	.L10
.L14:
	call	_Z12getDebugCharv
	movb	%al, -2(%rbp)
	cmpb	$36, -2(%rbp)
	je	.L22
.L11:
	cmpb	$35, -2(%rbp)
	je	.L23
.L12:
	movzbl	-3(%rbp), %edx
	movzbl	-2(%rbp), %eax
	addl	%edx, %eax
	movb	%al, -3(%rbp)
	movl	-8(%rbp), %eax
	cltq
	addq	-16(%rbp), %rax
	movzbl	-2(%rbp), %edx
	movb	%dl, (%rax)
	addl	$1, -8(%rbp)
.L10:
	cmpl	$398, -8(%rbp)
	setle	%al
	testb	%al, %al
	jne	.L14
	jmp	.L13
.L23:
	nop
.L13:
	movl	-8(%rbp), %eax
	cltq
	addq	-16(%rbp), %rax
	movb	$0, (%rax)
	cmpb	$35, -2(%rbp)
	jne	.L24
	call	_Z12getDebugCharv
	movb	%al, -2(%rbp)
	movsbl	-2(%rbp), %eax
	movl	%eax, %edi
	call	_Z3hexc
	sall	$4, %eax
	movb	%al, -1(%rbp)
	call	_Z12getDebugCharv
	movb	%al, -2(%rbp)
	movsbl	-2(%rbp), %eax
	movl	%eax, %edi
	call	_Z3hexc
	movl	%eax, %edx
	movzbl	-1(%rbp), %eax
	addl	%edx, %eax
	movb	%al, -1(%rbp)
	movzbl	-3(%rbp), %eax
	cmpb	-1(%rbp), %al
	je	.L16
	movl	remote_debug(%rip), %eax
	testl	%eax, %eax
	je	.L17
	movsbl	-1(%rbp), %ecx
	movsbl	-3(%rbp), %edx
	movq	stderr(%rip), %rax
	movq	-16(%rbp), %rsi
	movq	%rsi, %r8
	movl	$.LC0, %esi
	movq	%rax, %rdi
	movl	$0, %eax
	call	fprintf
.L17:
	movl	$45, %edi
	call	_Z12putDebugCharh
	jmp	.L24
.L16:
	movl	$43, %edi
	call	_Z12putDebugCharh
	movq	-16(%rbp), %rax
	addq	$2, %rax
	movzbl	(%rax), %eax
	cmpb	$58, %al
	jne	.L18
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	movl	%eax, %edi
	call	_Z12putDebugCharh
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	movl	%eax, %edi
	call	_Z12putDebugCharh
	movq	-16(%rbp), %rax
	addq	$3, %rax
	jmp	.L19
.L18:
	movq	-16(%rbp), %rax
.L19:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	_Z9getpacketv, .-_Z9getpacketv
	.globl	_Z9putpacketPc
	.type	_Z9putpacketPc, @function
_Z9putpacketPc:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movq	%rdi, -24(%rbp)
.L28:
	movl	$36, %edi
	call	_Z12putDebugCharh
	movb	$0, -2(%rbp)
	movl	$0, -8(%rbp)
	jmp	.L26
.L27:
	movzbl	-1(%rbp), %eax
	movzbl	%al, %eax
	movl	%eax, %edi
	call	_Z12putDebugCharh
	movzbl	-2(%rbp), %edx
	movzbl	-1(%rbp), %eax
	addl	%edx, %eax
	movb	%al, -2(%rbp)
	addl	$1, -8(%rbp)
.L26:
	movl	-8(%rbp), %eax
	cltq
	addq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movb	%al, -1(%rbp)
	cmpb	$0, -1(%rbp)
	setne	%al
	testb	%al, %al
	jne	.L27
	movl	$35, %edi
	call	_Z12putDebugCharh
	movsbl	-2(%rbp), %eax
	sarl	$4, %eax
	cltq
	movzbl	_ZL8hexchars(%rax), %eax
	movzbl	%al, %eax
	movl	%eax, %edi
	call	_Z12putDebugCharh
	movzbl	-2(%rbp), %eax
	movl	%eax, %edx
	sarb	$7, %dl
	shrb	$4, %dl
	addl	%edx, %eax
	andl	$15, %eax
	subb	%dl, %al
	movsbl	%al, %eax
	cltq
	movzbl	_ZL8hexchars(%rax), %eax
	movzbl	%al, %eax
	movl	%eax, %edi
	call	_Z12putDebugCharh
	call	_Z12getDebugCharv
	cmpl	$43, %eax
	setne	%al
	testb	%al, %al
	jne	.L28
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	_Z9putpacketPc, .-_Z9putpacketPc
	.globl	_Z11debug_errorPKcz
	.type	_Z11debug_errorPKcz, @function
_Z11debug_errorPKcz:
.LFB4:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$224, %rsp
	movq	%rsi, -168(%rbp)
	movq	%rdx, -160(%rbp)
	movq	%rcx, -152(%rbp)
	movq	%r8, -144(%rbp)
	movq	%r9, -136(%rbp)
	testb	%al, %al
	je	.L30
	movaps	%xmm0, -128(%rbp)
	movaps	%xmm1, -112(%rbp)
	movaps	%xmm2, -96(%rbp)
	movaps	%xmm3, -80(%rbp)
	movaps	%xmm4, -64(%rbp)
	movaps	%xmm5, -48(%rbp)
	movaps	%xmm6, -32(%rbp)
	movaps	%xmm7, -16(%rbp)
.L30:
	movq	%rdi, -216(%rbp)
	movl	remote_debug(%rip), %eax
	testl	%eax, %eax
	je	.L29
	movl	$8, -200(%rbp)
	movl	$48, -196(%rbp)
	leaq	16(%rbp), %rax
	movq	%rax, -192(%rbp)
	leaq	-176(%rbp), %rax
	movq	%rax, -184(%rbp)
	movq	stderr(%rip), %rax
	leaq	-200(%rbp), %rdx
	movq	-216(%rbp), %rcx
	movq	%rcx, %rsi
	movq	%rax, %rdi
	movl	$0, %eax
	call	fprintf
.L29:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE4:
	.size	_Z11debug_errorPKcz, .-_Z11debug_errorPKcz
	.globl	mem_fault_routine
	.bss
	.align 8
	.type	mem_fault_routine, @object
	.size	mem_fault_routine, 8
mem_fault_routine:
	.zero	8
	.local	_ZL7mem_err
	.comm	_ZL7mem_err,4,4
	.text
	.globl	_Z11set_mem_errv
	.type	_Z11set_mem_errv, @function
_Z11set_mem_errv:
.LFB5:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	$1, _ZL7mem_err(%rip)
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE5:
	.size	_Z11set_mem_errv, .-_Z11set_mem_errv
	.globl	_Z8get_charPc
	.type	_Z8get_charPc, @function
_Z8get_charPc:
.LFB6:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movzbl	(%rax), %eax
	movsbl	%al, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE6:
	.size	_Z8get_charPc, .-_Z8get_charPc
	.globl	_Z8set_charPci
	.type	_Z8set_charPci, @function
_Z8set_charPci:
.LFB7:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-12(%rbp), %eax
	movl	%eax, %edx
	movq	-8(%rbp), %rax
	movb	%dl, (%rax)
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	_Z8set_charPci, .-_Z8set_charPci
	.globl	_Z7mem2hexPcS_ii
	.type	_Z7mem2hexPcS_ii, @function
_Z7mem2hexPcS_ii:
.LFB8:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$40, %rsp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movl	%edx, -36(%rbp)
	movl	%ecx, -40(%rbp)
	cmpl	$0, -40(%rbp)
	je	.L36
	movq	$_Z11set_mem_errv, mem_fault_routine(%rip)
.L36:
	movl	$0, -8(%rbp)
	jmp	.L37
.L42:
	movq	-24(%rbp), %rax
	addq	$1, -24(%rbp)
	movq	%rax, %rdi
	call	_Z8get_charPc
	movb	%al, -1(%rbp)
	cmpl	$0, -40(%rbp)
	je	.L38
	movl	_ZL7mem_err(%rip), %eax
	testl	%eax, %eax
	je	.L38
	movl	$1, %eax
	jmp	.L39
.L38:
	movl	$0, %eax
.L39:
	testb	%al, %al
	je	.L40
	movq	-32(%rbp), %rax
	jmp	.L41
.L40:
	movzbl	-1(%rbp), %eax
	sarl	$4, %eax
	cltq
	movzbl	_ZL8hexchars(%rax), %edx
	movq	-32(%rbp), %rax
	movb	%dl, (%rax)
	addq	$1, -32(%rbp)
	movzbl	-1(%rbp), %eax
	andl	$15, %eax
	cltq
	movzbl	_ZL8hexchars(%rax), %edx
	movq	-32(%rbp), %rax
	movb	%dl, (%rax)
	addq	$1, -32(%rbp)
	addl	$1, -8(%rbp)
.L37:
	movl	-8(%rbp), %eax
	cmpl	-36(%rbp), %eax
	setl	%al
	testb	%al, %al
	jne	.L42
	movq	-32(%rbp), %rax
	movb	$0, (%rax)
	cmpl	$0, -40(%rbp)
	je	.L43
	movq	$0, mem_fault_routine(%rip)
.L43:
	movq	-32(%rbp), %rax
.L41:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	_Z7mem2hexPcS_ii, .-_Z7mem2hexPcS_ii
	.globl	_Z7hex2memPcS_ii
	.type	_Z7hex2memPcS_ii, @function
_Z7hex2memPcS_ii:
.LFB9:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$40, %rsp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movl	%edx, -36(%rbp)
	movl	%ecx, -40(%rbp)
	cmpl	$0, -40(%rbp)
	je	.L45
	movq	$_Z11set_mem_errv, mem_fault_routine(%rip)
.L45:
	movl	$0, -8(%rbp)
	jmp	.L46
.L51:
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movsbl	%al, %eax
	addq	$1, -24(%rbp)
	movl	%eax, %edi
	call	_Z3hexc
	sall	$4, %eax
	movb	%al, -1(%rbp)
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movsbl	%al, %eax
	addq	$1, -24(%rbp)
	movl	%eax, %edi
	call	_Z3hexc
	addb	%al, -1(%rbp)
	movzbl	-1(%rbp), %edx
	movq	-32(%rbp), %rax
	addq	$1, -32(%rbp)
	movl	%edx, %esi
	movq	%rax, %rdi
	call	_Z8set_charPci
	cmpl	$0, -40(%rbp)
	je	.L47
	movl	_ZL7mem_err(%rip), %eax
	testl	%eax, %eax
	je	.L47
	movl	$1, %eax
	jmp	.L48
.L47:
	movl	$0, %eax
.L48:
	testb	%al, %al
	je	.L49
	movq	-32(%rbp), %rax
	jmp	.L50
.L49:
	addl	$1, -8(%rbp)
.L46:
	movl	-8(%rbp), %eax
	cmpl	-36(%rbp), %eax
	setl	%al
	testb	%al, %al
	jne	.L51
	cmpl	$0, -40(%rbp)
	je	.L52
	movq	$0, mem_fault_routine(%rip)
.L52:
	movq	-32(%rbp), %rax
.L50:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE9:
	.size	_Z7hex2memPcS_ii, .-_Z7hex2memPcS_ii
	.globl	_Z13computeSignali
	.type	_Z13computeSignali, @function
_Z13computeSignali:
.LFB10:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	cmpl	$16, -20(%rbp)
	ja	.L54
	movl	-20(%rbp), %eax
	movq	.L70(,%rax,8), %rax
	jmp	*%rax
	.section	.rodata
	.align 8
	.align 4
.L70:
	.quad	.L55
	.quad	.L56
	.quad	.L54
	.quad	.L57
	.quad	.L58
	.quad	.L59
	.quad	.L60
	.quad	.L61
	.quad	.L62
	.quad	.L63
	.quad	.L64
	.quad	.L65
	.quad	.L66
	.quad	.L67
	.quad	.L68
	.quad	.L54
	.quad	.L69
	.text
.L55:
	movl	$8, -4(%rbp)
	jmp	.L71
.L56:
	movl	$5, -4(%rbp)
	jmp	.L71
.L57:
	movl	$5, -4(%rbp)
	jmp	.L71
.L58:
	movl	$16, -4(%rbp)
	jmp	.L71
.L59:
	movl	$16, -4(%rbp)
	jmp	.L71
.L60:
	movl	$4, -4(%rbp)
	jmp	.L71
.L61:
	movl	$8, -4(%rbp)
	jmp	.L71
.L62:
	movl	$7, -4(%rbp)
	jmp	.L71
.L63:
	movl	$11, -4(%rbp)
	jmp	.L71
.L64:
	movl	$11, -4(%rbp)
	jmp	.L71
.L65:
	movl	$11, -4(%rbp)
	jmp	.L71
.L66:
	movl	$11, -4(%rbp)
	jmp	.L71
.L67:
	movl	$11, -4(%rbp)
	jmp	.L71
.L68:
	movl	$11, -4(%rbp)
	jmp	.L71
.L69:
	movl	$7, -4(%rbp)
	jmp	.L71
.L54:
	movl	$7, -4(%rbp)
.L71:
	movl	-4(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	_Z13computeSignali, .-_Z13computeSignali
	.globl	_Z8hexToIntPPcPl
	.type	_Z8hexToIntPPcPl, @function
_Z8hexToIntPPcPl:
.LFB11:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movl	$0, -8(%rbp)
	movq	-32(%rbp), %rax
	movq	$0, (%rax)
	jmp	.L73
.L76:
	movq	-24(%rbp), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %eax
	movsbl	%al, %eax
	movl	%eax, %edi
	call	_Z3hexc
	movl	%eax, -4(%rbp)
	cmpl	$0, -4(%rbp)
	js	.L77
	movq	-32(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, %rdx
	salq	$4, %rdx
	movl	-4(%rbp), %eax
	cltq
	orq	%rax, %rdx
	movq	-32(%rbp), %rax
	movq	%rdx, (%rax)
	addl	$1, -8(%rbp)
	movq	-24(%rbp), %rax
	movq	(%rax), %rax
	leaq	1(%rax), %rdx
	movq	-24(%rbp), %rax
	movq	%rdx, (%rax)
.L73:
	movq	-24(%rbp), %rax
	movq	(%rax), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	setne	%al
	testb	%al, %al
	jne	.L76
	jmp	.L75
.L77:
	nop
.L75:
	movl	-8(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE11:
	.size	_Z8hexToIntPPcPl, .-_Z8hexToIntPPcPl
	.section	.rodata
.LC1:
	.string	"vector=%d, sr=0x%x, pc=0x%x\n"
.LC2:
	.string	"memory fault"
	.text
	.globl	handle_exception
	.type	handle_exception, @function
handle_exception:
.LFB12:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$64, %rsp
	movl	%edi, -52(%rbp)
	movl	-52(%rbp), %eax
	movl	%eax, gdb_i386vector(%rip)
	movl	remote_debug(%rip), %eax
	testl	%eax, %eax
	je	.L79
	movl	registers+32(%rip), %ecx
	movl	registers+36(%rip), %edx
	movl	-52(%rbp), %eax
	movl	%eax, %esi
	movl	$.LC1, %edi
	movl	$0, %eax
	call	printf
.L79:
	movl	-52(%rbp), %eax
	movl	%eax, %edi
	call	_Z13computeSignali
	movl	%eax, -8(%rbp)
	movq	$_ZL15remcomOutBuffer, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$84, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movl	-8(%rbp), %edx
	sarl	$4, %edx
	movslq	%edx, %rdx
	movzbl	_ZL8hexchars(%rdx), %edx
	movb	%dl, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movl	-8(%rbp), %edx
	andl	$15, %edx
	movslq	%edx, %rdx
	movzbl	_ZL8hexchars(%rdx), %edx
	movb	%dl, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movzbl	_ZL8hexchars+4(%rip), %edx
	movb	%dl, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$58, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movl	$0, %ecx
	movl	$4, %edx
	movq	%rax, %rsi
	movl	$registers+16, %edi
	call	_Z7mem2hexPcS_ii
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$59, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movzbl	_ZL8hexchars+5(%rip), %edx
	movb	%dl, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$58, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movl	$0, %ecx
	movl	$4, %edx
	movq	%rax, %rsi
	movl	$registers+20, %edi
	call	_Z7mem2hexPcS_ii
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$59, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movzbl	_ZL8hexchars+8(%rip), %edx
	movb	%dl, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$58, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movl	$0, %ecx
	movl	$4, %edx
	movq	%rax, %rsi
	movl	$registers+32, %edi
	call	_Z7mem2hexPcS_ii
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$59, (%rax)
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movb	$0, (%rax)
	movl	$_ZL15remcomOutBuffer, %edi
	call	_Z9putpacketPc
	movl	$0, -12(%rbp)
.L103:
	movb	$0, _ZL15remcomOutBuffer(%rip)
	call	_Z9getpacketv
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movzbl	(%rax), %edx
	movsbl	%dl, %edx
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	leal	-63(%rdx), %eax
	cmpl	$52, %eax
	ja	.L80
	movl	%eax, %eax
	movq	.L91(,%rax,8), %rax
	jmp	*%rax
	.section	.rodata
	.align 8
	.align 4
.L91:
	.quad	.L81
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L82
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L83
	.quad	.L80
	.quad	.L80
	.quad	.L84
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L85
	.quad	.L86
	.quad	.L80
	.quad	.L80
	.quad	.L87
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L89
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L80
	.quad	.L90
	.text
.L81:
	movb	$83, _ZL15remcomOutBuffer(%rip)
	movl	-8(%rbp), %eax
	sarl	$4, %eax
	cltq
	movzbl	_ZL8hexchars(%rax), %eax
	movb	%al, _ZL15remcomOutBuffer+1(%rip)
	movl	-8(%rbp), %eax
	movl	%eax, %edx
	sarl	$31, %edx
	shrl	$28, %edx
	addl	%edx, %eax
	andl	$15, %eax
	subl	%edx, %eax
	cltq
	movzbl	_ZL8hexchars(%rax), %eax
	movb	%al, _ZL15remcomOutBuffer+2(%rip)
	movb	$0, _ZL15remcomOutBuffer+3(%rip)
	jmp	.L80
.L86:
	movl	remote_debug(%rip), %eax
	testl	%eax, %eax
	sete	%al
	movzbl	%al, %eax
	movl	%eax, remote_debug(%rip)
	jmp	.L80
.L87:
	movl	$0, %ecx
	movl	$64, %edx
	movl	$_ZL15remcomOutBuffer, %esi
	movl	$registers, %edi
	call	_Z7mem2hexPcS_ii
	jmp	.L80
.L82:
	movq	-32(%rbp), %rax
	movl	$0, %ecx
	movl	$64, %edx
	movl	$registers, %esi
	movq	%rax, %rdi
	call	_Z7hex2memPcS_ii
	movw	$19279, _ZL15remcomOutBuffer(%rip)
	movb	$0, _ZL15remcomOutBuffer+2(%rip)
	jmp	.L80
.L84:
	leaq	-24(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	_Z8hexToIntPPcPl
	testl	%eax, %eax
	je	.L92
	movq	-32(%rbp), %rax
	movzbl	(%rax), %edx
	cmpb	$61, %dl
	sete	%dl
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	testb	%dl, %dl
	je	.L92
	movl	$1, %eax
	jmp	.L93
.L92:
	movl	$0, %eax
.L93:
	testb	%al, %al
	je	.L94
	movq	-24(%rbp), %rax
	testq	%rax, %rax
	js	.L94
	movq	-24(%rbp), %rax
	cmpq	$15, %rax
	jg	.L94
	movq	-24(%rbp), %rax
	salq	$2, %rax
	leaq	registers(%rax), %rsi
	movq	-32(%rbp), %rax
	movl	$0, %ecx
	movl	$4, %edx
	movq	%rax, %rdi
	call	_Z7hex2memPcS_ii
	movw	$19279, _ZL15remcomOutBuffer(%rip)
	movb	$0, _ZL15remcomOutBuffer+2(%rip)
	jmp	.L80
.L94:
	movl	$3223621, _ZL15remcomOutBuffer(%rip)
	jmp	.L80
.L89:
	leaq	-48(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	_Z8hexToIntPPcPl
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L95
	movq	-32(%rbp), %rax
	movzbl	(%rax), %edx
	cmpb	$44, %dl
	sete	%dl
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	testb	%dl, %dl
	je	.L95
	leaq	-40(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	_Z8hexToIntPPcPl
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L95
	movq	$0, -32(%rbp)
	movl	$0, _ZL7mem_err(%rip)
	movq	-40(%rbp), %rax
	movl	%eax, %edx
	movq	-48(%rbp), %rax
	movl	$1, %ecx
	movl	$_ZL15remcomOutBuffer, %esi
	movq	%rax, %rdi
	call	_Z7mem2hexPcS_ii
	movl	_ZL7mem_err(%rip), %eax
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L95
	movl	$3354693, _ZL15remcomOutBuffer(%rip)
	movl	$.LC2, %edi
	movl	$0, %eax
	call	_Z11debug_errorPKcz
.L95:
	movq	-32(%rbp), %rax
	testq	%rax, %rax
	je	.L104
	movl	$3223621, _ZL15remcomOutBuffer(%rip)
	jmp	.L104
.L83:
	leaq	-48(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	_Z8hexToIntPPcPl
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L97
	movq	-32(%rbp), %rax
	movzbl	(%rax), %edx
	cmpb	$44, %dl
	sete	%dl
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	testb	%dl, %dl
	je	.L97
	leaq	-40(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	_Z8hexToIntPPcPl
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L97
	movq	-32(%rbp), %rax
	movzbl	(%rax), %edx
	cmpb	$58, %dl
	sete	%dl
	addq	$1, %rax
	movq	%rax, -32(%rbp)
	testb	%dl, %dl
	je	.L97
	movl	$0, _ZL7mem_err(%rip)
	movq	-40(%rbp), %rax
	movl	%eax, %edx
	movq	-48(%rbp), %rax
	movq	%rax, %rsi
	movq	-32(%rbp), %rax
	movl	$1, %ecx
	movq	%rax, %rdi
	call	_Z7hex2memPcS_ii
	movl	_ZL7mem_err(%rip), %eax
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L98
	movl	$3354693, _ZL15remcomOutBuffer(%rip)
	movl	$.LC2, %edi
	movl	$0, %eax
	call	_Z11debug_errorPKcz
	jmp	.L99
.L98:
	movw	$19279, _ZL15remcomOutBuffer(%rip)
	movb	$0, _ZL15remcomOutBuffer+2(%rip)
.L99:
	movq	$0, -32(%rbp)
.L97:
	movq	-32(%rbp), %rax
	testq	%rax, %rax
	je	.L105
	movl	$3289157, _ZL15remcomOutBuffer(%rip)
	jmp	.L105
.L90:
	movl	$1, -12(%rbp)
.L85:
	leaq	-48(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	_Z8hexToIntPPcPl
	testl	%eax, %eax
	setne	%al
	testb	%al, %al
	je	.L101
	movq	-48(%rbp), %rax
	movl	%eax, registers+32(%rip)
.L101:
	movl	registers+32(%rip), %eax
	movl	%eax, -4(%rbp)
	movl	registers+36(%rip), %eax
	andb	$254, %ah
	movl	%eax, registers+36(%rip)
	cmpl	$0, -12(%rbp)
	je	.L102
	movl	registers+36(%rip), %eax
	orb	$1, %ah
	movl	%eax, registers+36(%rip)
.L102:
	call	_Z20_returnFromExceptionv
	jmp	.L80
.L104:
	nop
	jmp	.L80
.L105:
	nop
.L80:
	movl	$_ZL15remcomOutBuffer, %edi
	call	_Z9putpacketPc
	jmp	.L103
	.cfi_endproc
.LFE12:
	.size	handle_exception, .-handle_exception
	.globl	_Z15set_debug_trapsv
	.type	_Z15set_debug_trapsv, @function
_Z15set_debug_trapsv:
.LFB13:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	$remcomStack+9996, stackPtr(%rip)
	movl	$catchException0, %esi
	movl	$0, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException1, %esi
	movl	$1, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException3, %esi
	movl	$3, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException4, %esi
	movl	$4, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException5, %esi
	movl	$5, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException6, %esi
	movl	$6, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException7, %esi
	movl	$7, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException8, %esi
	movl	$8, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException9, %esi
	movl	$9, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException10, %esi
	movl	$10, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException11, %esi
	movl	$11, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException12, %esi
	movl	$12, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException13, %esi
	movl	$13, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException14, %esi
	movl	$14, %edi
	call	_Z16exceptionHandleriPFvvE
	movl	$catchException16, %esi
	movl	$16, %edi
	call	_Z16exceptionHandleriPFvvE
	movb	$1, _ZL11initialized(%rip)
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE13:
	.size	_Z15set_debug_trapsv, .-_Z15set_debug_trapsv
	.globl	_Z10breakpointv
	.type	_Z10breakpointv, @function
_Z10breakpointv:
.LFB14:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movzbl	_ZL11initialized(%rip), %eax
	testb	%al, %al
	je	.L107
#APP
# 945 "i386-stub.cc" 1
	   int $3
# 0 "" 2
#NO_APP
.L107:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE14:
	.size	_Z10breakpointv, .-_Z10breakpointv
	.section	.rodata
	.align 16
	.type	_ZL8hexchars, @object
	.size	_ZL8hexchars, 17
_ZL8hexchars:
	.string	"0123456789abcdef"
	.ident	"GCC: (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3"
	.section	.note.GNU-stack,"",@progbits
