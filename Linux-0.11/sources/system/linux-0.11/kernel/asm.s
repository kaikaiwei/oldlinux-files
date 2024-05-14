/*
 *  linux/kernel/asm.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * asm.s contains the low-level code for most hardware faults.
 * page_exception is handled by the mm, so that isn't here. This
 * file also handles (hopefully) fpu-exceptions due to TS-bit, as
 * the fpu must be properly saved/resored. This hasn't been tested.
 */

/**
 * 包括大部分cpu探测到的异常故障处理的底层代码，包括数字协处理器（FPU）的异常处理。
 * 主要处理方式是在中断处理程序中，卓当调用相应的c函数程序，显示出错位置和出错号，然后退出中断
 * 本代码主要设计0-16号中断处理，17-31保留。
 */

# 全局函数声明，原型在traps.c中。
.globl _divide_error,_debug,_nmi,_int3,_overflow,_bounds,_invalid_op
.globl _double_fault,_coprocessor_segment_overrun
.globl _invalid_TSS,_segment_not_present,_stack_segment
.globl _general_protection,_coprocessor_error,_irq13,_reserved

# int0， 除0异常。
_divide_error:
	pushl $_do_divide_error	# 首先把处理函数入栈
no_error_code:				# 这里是无出错号的处理。
	xchgl %eax,(%esp)		# 将esp保存到eax中，eax被交换入栈。
	pushl %ebx				# ebx入栈
	pushl %ecx				# ecx入栈
	pushl %edx				# edx入栈
	pushl %edi				# edi入栈
	pushl %esi				# esi入栈
	pushl %ebp				# ebp入栈
	push %ds				# ds入栈
	push %es				# es入栈
	push %fs				# fs入栈
	pushl $0		# "error code"， 出错号入栈	
	lea 44(%esp),%edx		# 取原调用返回地址处堆栈指针位置，并压入堆栈。esp+44字节的地址复制给edx。
	pushl %edx				# edx（原调用函数返回地址）入栈
	movl $0x10,%edx			# edx为0x10， 表示内核数据段选择符。
	mov %dx,%ds				# ds为内核数据段选择符
	mov %dx,%es				# es为内核数据段选择符
	mov %dx,%fs				# fs为内核数据段选择符
	call *%eax				# 调用eax，也就是_do_divide_error
	addl $8,%esp			# c函数返回后，esp+8，刚好移除掉连个参数的位置。即addr（原调用函数的返回地址）和error_code
	pop %fs					# 出栈操作
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

_debug:
	pushl $_do_int3		# _do_debug
	jmp no_error_code

_nmi:
	pushl $_do_nmi
	jmp no_error_code

_int3:
	pushl $_do_int3
	jmp no_error_code

_overflow:
	pushl $_do_overflow
	jmp no_error_code

_bounds:
	pushl $_do_bounds
	jmp no_error_code

_invalid_op:
	pushl $_do_invalid_op
	jmp no_error_code

_coprocessor_segment_overrun:
	pushl $_do_coprocessor_segment_overrun
	jmp no_error_code

_reserved:
	pushl $_do_reserved
	jmp no_error_code

# 下面用于处理协处理器执行完一个操作是就会发出irq13中断信号，以通知cpu操作2岸城
_irq13:					# int45（0x20+13），数字协处理器发出的中断
	pushl %eax			
	xorb %al,%al		# 80387在执行计算时，cpu会等待其操作的完成。al为0
	outb %al,$0xF0		# 将al
	movb $0x20,%al		# al=0x20
	outb %al,$0x20		# 向主中断控制器发送eoi（中断结束）信号
	jmp 1f				# 这两个指令起到延时作用
1:	jmp 1f
1:	outb %al,$0xA0		# 将8025从中断控制芯片发送eoi（中断结束）信号
	popl %eax			# eax出栈
	jmp _coprocessor_error	# 调用kernel/system_call.s中_coprocessor_error函数

# 以下中断在调用时会在中断返回地址后面将出错号压入堆栈，因此返回时也需要将错误号弹出
# int8 双输错故障。
_double_fault:
	pushl $_do_double_fault
error_code:
	xchgl %eax,4(%esp)		# error code <-> %eax
	xchgl %ebx,(%esp)		# &function <-> %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl %eax			# error code
	lea 44(%esp),%eax		# offset， 程序返回地址处堆栈指针位置入栈
	pushl %eax
	movl $0x10,%eax			# 置内核数据段选择符
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	call *%ebx				# 调用相应的c函数，其参数已尽刚入栈。
	addl $8,%esp			# 堆栈指针重新指向栈中方fs内容的位置
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

_invalid_TSS:			# int10 无效的任务状态段
	pushl $_do_invalid_TSS
	jmp error_code

_segment_not_present:	# int11 段不存在
	pushl $_do_segment_not_present
	jmp error_code

_stack_segment:			# int12 堆栈段错误
	pushl $_do_stack_segment
	jmp error_code

_general_protection:	# int13 一般保护性错误
	pushl $_do_general_protection
	jmp error_code

