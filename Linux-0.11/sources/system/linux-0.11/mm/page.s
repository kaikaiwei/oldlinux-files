/*
 *  linux/mm/page.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * page.s contains the low-level page-exception code.
 * the real work is done in mm.c
 * 包含低级别的页面异常处理函数。 真正工作的事在mm.c中。
 * 内存页异常中断处理过程 int 14的处理。 主要是缺页和页写保护的处理。
 */

.globl _page_fault

_page_fault:
	xchgl %eax,(%esp)	# 取出错号到eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%edx		# 置内核数据段选择符
	mov %dx,%ds
	mov %dx,%es
	mov %dx,%fs
	movl %cr2,%edx		# 取引起页面异常的线性地址
	pushl %edx			# 将该线性地址和出错号压入堆栈， 作为调用函数的参数
	pushl %eax
	testl $1,%eax		# 测试标志P，如果不是缺页引起的异常则跳转_do_no_page
	jne 1f
	call _do_no_page	# 调用缺页处理函数
	jmp 2f
1:	call _do_wp_page	# 调用写保护异常函数
2:	addl $8,%esp		# 丢弃也如堆栈的两个参数
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret
