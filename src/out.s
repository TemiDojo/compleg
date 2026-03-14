	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $24, %esp
	pushl %ebx
.L0:
	movl $5, -8(%ebp)
	jmp .L2
.L1:
	movl $40, %eax
	popl %ebx
	leave
	ret
.L2:
	movl -8(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .L3
	jmp .L4
.L3:
	movl -8(%ebp), %ebx
	addl $1, %ebx
	movl %ebx, -8(%ebp)
	movl -8(%ebp), %ebx
	movl %ebx, %ecx
	cmpl $15, %ecx
	jg .L5
	jmp .L6
.L4:
	movl -8(%ebp), %ebx
	pushl %ecx
	pushl %edx
	pushl %ebx
	call print
	addl $4, %esp
	popl %edx
	popl %ecx
	pushl %ecx
	pushl %edx
	pushl $15
	call print
	addl $4, %esp
	popl %edx
	popl %ecx
	pushl %ecx
	pushl %edx
	pushl $25
	call print
	addl $4, %esp
	popl %edx
	popl %ecx
	jmp .L1
.L5:
	jmp .L7
.L6:
	jmp .L7
.L7:
	jmp .L2
.L8:
	jmp .L1
