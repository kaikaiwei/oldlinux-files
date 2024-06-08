/*
 *  linux/kernel/rs_io.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	rs_io.s
 *  rs232串行驱动程序的中断调用处理程序
 * This module implements the rs232 io interrupts.
 */

# 文本段
.text
# 全局函数符号。
.globl _rs1_interrupt,_rs2_interrupt

# 读写缓冲队列大小
size	= 1024				/* must be power of two !
					   and must match the value
					   in tty_io.c!!! */

/* these are the offsets into the read/write buffer structures */
/* 对应定义在include/linux/tty.h文件中tty_queue结构中各变量的偏移量 */
rs_addr = 0		# 串行端口号字段偏移。 
head = 4		# 缓冲区中头指针的字段偏移量
tail = 8		# 缓冲区中尾指针的字段偏移量
proc_list = 12	# 等待该缓冲的进程字段偏移量
buf = 16		# 缓冲区字段偏移

# 当队列里还剩256个字符空间（WAKEUP_CHARS), 我们就可以写
startup	= 256		/* chars left in write queue when we restart it */

/*
 * These are the actual interrupt routines. They look where
 * the interrupt is coming from, and take appropriate action.
 * 这些是十几的中断程序。程序首先检查中断的来源。 然后执行相应的处理。
 */
.align 2		# 4字节对齐
_rs1_interrupt:		# 串口1中断处理程序入口
	pushl $_table_list+8		# tty 表中对应串行口1的读写缓冲指针地址入栈。tty_io.c中。
	jmp rs_int					# 跳转到46行继续执行
.align 2		# 4字节对齐
_rs2_interrupt:					# 串口2中断处理程序入口
	pushl $_table_list+16		# tty表中对应串口2的读写缓冲指针地址入栈。 tty_io.c中
rs_int:							# 中断处理程序
	pushl %edx					# 保存寄存器的值
	pushl %ecx
	pushl %ebx
	pushl %eax
	push %es					# 保存es，ds。 然后使用内核数据段
	push %ds		/* as this is an interrupt, we cannot */
	pushl $0x10		/* know that bs is ok. Load it */
	pop %ds
	pushl $0x10
	pop %es
	movl 24(%esp),%edx			# 将缓冲队列指针地址存入edx寄存器
	movl (%edx),%edx			# 取读队列指针地址到edx中。
	movl rs_addr(%edx),%edx		# 取串行口1的端口号到edx中
	addl $2,%edx		/* interrupt ident. reg。 edx执行中断表示符寄存器*/
rep_int:						# 中断标识寄存器是0x3fa（0x2fa）。
	xorl %eax,%eax				# eax 清零
	inb %dx,%al					# 取中断标识字节。 用来判断中断来源，有4中中断情况
	testb $1,%al				# 判断有误带处理的中断。位0=1无中断。 位0=0有中断
	jne end						# 没有中断，跳转到结束位置
	cmpb $6,%al		/* this shouldn't happen, but ...  者不会发生 */
	ja end						# al的值大于6， 则跳转到end
	movl 24(%esp),%ecx			# ecx为缓冲队列指针地址
	pushl %edx					# 将端口号入栈。
	subl $2,%edx				# 端口号减去2， 0x3f8（0x2f8）
	call jmp_table(,%eax,2)		/* NOTE! not *4, bit0 is 0 already，当有带处理中断时，al的位0=0，位2=1（中断类型），相当于一境将中断类型乘以2，这里在成2. 得到跳转表对应个中断类型地址 */
	popl %edx					# edx出栈
	jmp rep_int					# 跳转，继续判断有无要处理的中断。
end:	movb $0x20,%al			
	outb %al,$0x20		/* EOI， 项中断控制器发送结束中断指令EOI */
	pop %ds
	pop %es
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	addl $4,%esp		# jump over _table_list entry
	iret

# 各中断类型处理程序的跳转表，有4种中断来源
# modem 状态变化中断，写字符中断，读字符中断，线路状态有问题中断
jmp_table:
	.long modem_status,write_char,read_char,line_status

.align 2	# 4字节对齐
modem_status:
	addl $6,%edx		/* clear intr by reading modem status reg */
	inb %dx,%al			# 读modem状态寄存器进行复位。0x3fe
	ret

.align 2	
line_status:			
	addl $5,%edx		/* clear intr by reading line status reg. */
	inb %dx,%al			# 通过读线路状态寄存器进行复位。 0x3fd
	ret

.align 2
read_char:				# 读字符
	inb %dx,%al			# 读取字符  al 。 ecx是缓冲队列头指针
	movl %ecx,%edx				# edx=ecx
	subl $_table_list,%edx		# 缓冲队列指针表收地址-当前串口队列地址=edx
	shrl $3,%edx				# edx/8， 对于串口1，结果是1. 对于串口2，结果是2
	movl (%ecx),%ecx		# read-queue  取读缓冲队列结构地址到ecx中
	movl head(%ecx),%ebx		# 取缓冲区头指针位置到ebx中
	movb %al,buf(%ecx,%ebx)		# 将字符放在缓冲区中头指针的位置
	incl %ebx					# 头指针向前移动一个
	andl $size-1,%ebx			# 用缓冲区大小对头指针进行取模操作
	cmpl tail(%ecx),%ebx		# 头指针和尾指针比较
	je 1f						# 如果相等，表示缓冲区满。向前跳转到1号位置。
	movl %ebx,head(%ecx)		# 保存修改过的头指针
1:	pushl %edx					# 将串口号压入堆栈。 作为参数
	call _do_tty_interrupt		# 调用tty中断处理c函数
	addl $4,%esp				# 对齐入栈参数，并返回。
	ret

.align 2 	# 4字节对齐
write_char:					# 写入到写缓存队列
	movl 4(%ecx),%ecx		# write-queue， 取缓冲队列地址结构到ecx中。
	movl head(%ecx),%ebx	# 取头指针到ebx中
	subl tail(%ecx),%ebx	# 取尾指针减去ebx的值，然后放到ebx中
	andl $size-1,%ebx		# nr chars in queue。 对指针进行取模运算
	je write_buffer_empty	# 如果头指针=尾指针，表示写队列无字符
	cmpl $startup,%ebx		# 队列中字符超过256个
	ja 1f					# 超过，跳转处理
	movl proc_list(%ecx),%ebx	# wake up sleeping process	唤醒等待队列， 等待进程头指针
	testl %ebx,%ebx			# is there any?	如果当前有等待进程
	je 1f					# 是空的
	movl $0,(%ebx)			# 否则，ebx=0， 将进程设置为可运行状态。
1:	movl tail(%ecx),%ebx		# 将尾指针的位置给ebx
	movb buf(%ecx,%ebx),%al		# 从缓冲尾指针处，取一个字符到al
	outb %al,%dx				# 向端口0x3f8（0x2f8） 送出道保持寄存器中
	incl %ebx					# 尾指针前移
	andl $size-1,%ebx			# 如果尾指针到达缓冲区末端，折回处理
	movl %ebx,tail(%ecx)		# 保存已经修改的尾指针
	cmpl head(%ecx),%ebx		# 比较尾指针和头指针的位置，
	je write_buffer_empty		# 如果相等，表示队列空了，跳转到空队列处理
	ret
.align 2	# 4字节对齐
write_buffer_empty:				# 唤醒等待的进程
	movl proc_list(%ecx),%ebx	# wake up sleeping process。 取等待该队列的进程的指针
	testl %ebx,%ebx			# is there any? 如果ebx位0，表示没有等待的进程
	je 1f					# 没有，向前跳转到1号位置
	movl $0,(%ebx)			# 有等待的进程，ebx=0，0表示进程可运行，也就是唤醒进程
1:	incl %edx				# 增加edx的值。表示指向端口0x3f9（0x2f9）
	inb %dx,%al				# 读取中断允许寄存器到al中。
	jmp 1f					# 延时处理
1:	jmp 1f
1:	andb $0xd,%al		/* disable transmit interrupt 屏蔽发送保持寄存器空中断（位1） */
	outb %al,%dx		# al写入端口0x3f9（0x2f9）
	ret
