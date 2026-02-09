	.text
	.globl	func
	.type	func, @function
func:
## %bb.0:
	pushl %ebp
	movl %esp, %ebp
	subl $12, %esp
	pushl %ebx
	movl -8(%ebp), %ebx
	movl %ebx, 0(%ebp)
	movl 0(%ebp), %ecx
	movl %ecx, -12(%ebp)
	jmp ## %bb.1:
## %bb.1:
	movl -12(%ebp), %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
