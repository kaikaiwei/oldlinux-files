/*
 *  linux/kernel/system_call.s
 *
 *  (C) 1991  Linus Torvalds
 */

 /**
  * 本程序主要实现系统调用中断int 0x80的入口处理过程，以及信号检测处理。同时给出两个系统功能的底层接口：sys_execve，sys_fork。
  * 还列出了处理类似的协处理器错误（int 16）， 设备不存在（int 7）， 时钟中断（int 32），硬盘中断（int46），软盘中断（int38）的中断处理程序
  * 系统调用的c语言处理函数分布在整个linux内核代码中，由include/linux/sys.h头文件中的系统函数指针数组表来匹配。
  */

/*
 *  system_call.s  contains the system-call low-level handling routines.
 * This also contains the timer-interrupt handler, as some of the code is
 * the same. The hd- and flopppy-interrupts are also here.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call. Ordinary interrupts
 * don't handle signal-recognition, as that would clutter them up totally
 * unnecessarily.
 *
 * Stack layout in 'ret_from_system_call':  从系统调用返回的堆栈内容。
 *
 *	 0(%esp) - %eax
 *	 4(%esp) - %ebx
 *	 8(%esp) - %ecx
 *	 C(%esp) - %edx
 *	10(%esp) - %fs
 *	14(%esp) - %es
 *	18(%esp) - %ds
 *	1C(%esp) - %eip
 *	20(%esp) - %cs
 *	24(%esp) - %eflags
 *	28(%esp) - %oldesp		
 *	2C(%esp) - %oldss
 */

SIG_CHLD	= 17	# 子进程停止或结束的的信号量

EAX		= 0x00			#堆栈中各个寄存器的偏移位置
EBX		= 0x04
ECX		= 0x08
EDX		= 0x0C
FS		= 0x10
ES		= 0x14
DS		= 0x18
EIP		= 0x1C
CS		= 0x20
EFLAGS		= 0x24
OLDESP		= 0x28		# 当特权级别有变化时候
OLDSS		= 0x2C

state	= 0		# these are offsets into the task-struct. 进程状态码
counter	= 4		# 任务运行时间计数（递减），滴答数，运行时间片
priority = 8	# 运行优先级。任务开始运行时counter = priority， 越大则运行时间越长
signal	= 12		# 信号位图，每个比特为代表一种信号，信号值=位偏移量+1
sigaction = 16		# MUST be 16 (=len of sigaction). sigaction结构长度必须是16字节。信号执行属性结构数组的偏移量，对应信号将要执行的操作和标志信息
blocked = (33*16)	# 受阻信号位图的偏移量

# offsets within sigaction sigaction结构中的偏移量，kernel/signal.h
sa_handler = 0
sa_mask = 4
sa_flags = 8
sa_restorer = 12

nr_system_calls = 72	# 系统调用的总数

/* 
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 * 全局函数地址
 */
.globl _system_call,_sys_fork,_timer_interrupt,_sys_execve
.globl _hd_interrupt,_floppy_interrupt,_parallel_interrupt
.globl _device_not_available, _coprocessor_error

.align 2		# 4字节对齐
bad_sys_call:		# 错误的系统调用号，从这里返回
	movl $-1,%eax	# eax=-1
	iret			# 通过iret退出中断， i表示返回的是一个int类型（4字节）
.align 2			# 4字节对齐
reschedule:			# 重新执行调度入口。 schedule， kernel/sched.c
	pushl $ret_from_sys_call	# 将ret_from_sys_call的地址入栈
	jmp _schedule				# 调用schedule函数
.align 2			# 4字节对齐
_system_call:						# 系统调用， int 80 linux系统调用入口点。eax是调用号
	cmpl $nr_system_calls-1,%eax	# 比较调用号和最大点用号
	ja bad_sys_call					# 调用号大于最大调用号，调用错误的系统调用号进行执行。
	push %ds						# 保存原寄存器的值
	push %es
	push %fs
	pushl %edx						# 系统调用参数
	pushl %ecx		# push %ebx,%ecx,%edx as parameters
	pushl %ebx		# to the system call
	movl $0x10,%edx		# set up ds,es to kernel space	设置内核空间段寄存器。 ds，es。
	mov %dx,%ds
	mov %dx,%es
	movl $0x17,%edx		# fs points to local data space	fs执行局部数据段空间。
	mov %dx,%fs
	call _sys_call_table(,%eax,4) # 调用_sys_call_table + eax * 4, sys_call_table在include/sys.h中
	pushl %eax				# 把系统调用的返回值入栈
	movl _current,%eax		# 取当前数据结构地址
	cmpl $0,state(%eax)		# state	查看当前任务运行状态。如果不在就绪状态（state不等于0），就重新调度。
	jne reschedule			# 重新执行调度入口。
	cmpl $0,counter(%eax)		# counter 如果该任务的是就绪态，时间片为0，也重新调度。
	je reschedule
ret_from_sys_call:			# 从系统调用C函数返回后，对信号进行识别处理。
	movl _current,%eax		# task[0] cannot have signals。   eax=_current
	cmpl _task,%eax			# 比较eax和_task(代表0号进程，这是个数组)。
	je 3f					# 向前跳转到标号3. 
	cmpw $0x0f,CS(%esp)		# was old code segment supervisor ?  如果原堆栈段选择符的来判断是否是内核任务段。rpl=3, 局部表，第一个段（代码段）。如果不是则跳转退出中断程序。
	jne 3f
	cmpw $0x17,OLDSS(%esp)		# was stack segment = 0x17 ?  如果原是堆栈段选择符不是0x17（不在用户数据段中）， 也退出。
	jne 3f
	movl signal(%eax),%ebx		# 取信号位图（32位，每位代表1中信号），然后用任务结构中的信号阻塞（屏蔽）码，阻塞不允许的信号位，取得数值最小的信号值，再把原信号值给途中对应的信号复位（置0）。
	movl blocked(%eax),%ecx		# 取屏蔽信号位图
	notl %ecx					# ecx每位取反
	andl %ebx,%ecx				# 获得许可的信号位图。
	bsfl %ecx,%ecx				# 从低位（位0）开始扫描，看是否有1的位。如果有，则ecx保留改位的偏移值。如果没有则跳转退出。
	je 3f						
	btrl %ecx,%ebx				# 复位该信号， ebx还有原signal位图
	movl %ebx,signal(%eax)		# 重新保存signal位图信息  curren->signal 
	incl %ecx					# 将信号调整位从1开始的数（1-32）
	pushl %ecx					# 将信号值入栈作为调用do_signal的参数之一。
	call _do_signal				# 调用do_signal
	popl %eax					# 弹出信号值
3:	popl %eax					# 退出中断处理程序
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret

# int16 处理些处理其发出的错误，转换为执行c函数math_error(). kernel/math/math_emulate.c, 之后跳转到ret_from_sys_call处执行
.align 2				# 4 字节对齐
_coprocessor_error:		# 
	push %ds			# 保存内核段寄存器
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax		# 设置内核空间段寄存器。 ds，es。
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax		# 设置fs 寄存器指向局部数据段空间
	mov %ax,%fs
	pushl $ret_from_sys_call	# 将ret_from_sys_call入栈，然后调用_math_error
	jmp _math_error

# int7 设备不存在协处理器不存在
# 若控制寄存器cr0的EM标志置位，则当cpu执行一个转移指令是就会引发该中断。这样就有机会让这个中断处理程序模拟多个转移指令。
# cr0的ts标志是在cpu执行任务转换时设置的。
# ts 可以用来确定什么时候协处理器中的内容与cpu正在执行的任务不匹配了。
# 当cpu运行一个转移指令时发现ts置位了，就会引发该中断。此时就应该回复新任务的协处理器执行状态（178行）。 参见kernel/sched.c中说明。
.align 2				# 4字节对齐
_device_not_available:
	push %ds			# 保存内核段寄存器
	push %es
	push %fs
	pushl %edx			# 保存参数寄存器
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax		# 设置内核空间段寄存器 ds，es
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax		# 设置fs寄存器执行局部数据段空间
	mov %ax,%fs
	pushl $ret_from_sys_call	# 将ret_from_sys_call的地址入栈，作为返回地址
	clts				# clear TS so that we can use math。 清除ts标志
	movl %cr0,%eax			# eax为cr0（控制寄存器）中内容
	testl $0x4,%eax			# EM (math emulation bit)  如果em标志置位
	je _math_state_restore	# 跳转到_math_state_restore， 保存协处理器状态，如果不是协处理器造成的中断，则回复新任务协处理器状态。kernel/sched.c
	pushl %ebp
	pushl %esi
	pushl %edi
	call _math_emulate		# 调用c函数math_emulate, kernel/math/math_emulate.c 
	popl %edi
	popl %esi
	popl %ebp
	ret						# 这里会跳转到ret_from_sys_call中执行。

# int32， int 0x20 时钟中断处理程序。中断频率被设置为100Hz（include/linux/sched.h）
# 定时芯片8253/8254时在kernel/sched.c中初始化的。因此这里jiffies每10毫秒增加1，
# 这段代码将jniffies增加1，并发送结束中断指令给8259控制器，然后用当前特权级别作为参数调用c函数do_timer。 当调用返回时会转去检查并处理信号。
.align 2
_timer_interrupt:
	push %ds		# save ds,es and put kernel data space
	push %es		# into them. %fs is used by _system_call
	push %fs
	pushl %edx		# we save %eax,%ecx,%edx as gcc doesn't
	pushl %ecx		# save those across function calls. %ebx
	pushl %ebx		# is saved as we use that in ret_sys_call
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax		# fs被设置为局部数据段（出错程序的数据段）
	mov %ax,%fs
	incl _jiffies		# 增加_jiffies值。
	movb $0x20,%al		# EOI to interrupt controller #1 这里发指令结束该硬件中断。
	outb %al,$0x20		# 调用outb，将操作字ocw2送0x20端口
	movl CS(%esp),%eax	# 取出当前特权级别。
	andl $3,%eax		# %eax is CPL (0 or 3, 0=supervisor)
	pushl %eax			# 
	call _do_timer		# 'do_timer(long CPL)' does everything from
	addl $4,%esp		# task switching to accounting ... esp+4， 表示栈会退4个，刚好抛出eax。
	jmp ret_from_sys_call

# 这是sys_execve（）系统调用，去中断调用程序代码指针作为参数调用c函数do_execve（）。 在fs/exec.c中
.align 2
_sys_execve:
	lea EIP(%esp),%eax	# 保存中断调用程序代码指针
	pushl %eax
	call _do_execve
	addl $4,%esp
	ret

# sys_fork系统调用，用于创建子进程。时system_call的功能2. 原型在include/include/sys.h中。 
# 首先调用_find_empty_process，取得一个进程号pid。如果为负数表明当前任务数组已经满了。然后调用copy_process复制进程。
.align 2
_sys_fork:
	call _find_empty_process	# 查找未使用的进程号
	testl %eax,%eax				# 如果小于0
	js 1f
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call _copy_process			# 调用copy_process 函数
	addl $20,%esp				# 参数出栈
1:	ret

# int 46 硬盘中断处理程序，硬件中断控制请求 irq14
# 当硬盘完成操作或者出错就会发出这个信号。 kernel/blk_drv/hd.c。 
# 首先向8259A中断控制从芯片发送结束硬件中断指令（BOI）， 然后取变量do_hd中的函数指针放入edx寄存器中，
# 并设置do_hd为NULL， 接着判断edx函数指针是否为空。
# 如果为空，则给edx赋值执行unexpected_hd_interrupt（）， 用于显示出错信息。
# 随后向8259A主芯片送EOI指令，并调用edx中指针执行的函数：read_intr（），write_intr（），或unexcepted_hd_interrupt（）
_hd_interrupt:
	pushl %eax			# 寄存器放入栈中保存，用来当作参数
	pushl %ecx
	pushl %edx
	push %ds			# 保存段寄存器
	push %es
	push %fs
	movl $0x10,%eax		# 使用内核段寄存器
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax		# fs被设置为局部数据段（出错程序的数据段）
	mov %ax,%fs
	movb $0x20,%al
	outb %al,$0xA0		# EOI to interrupt controller #1
	jmp 1f			# give port chance to breathe。 两个延迟调用
1:	jmp 1f
1:	xorl %edx,%edx		# edx清0
	xchgl _do_hd,%edx	# edx=do_hd， do_hd是函数指针，被赋值为read_intr()或write_intr()
	testl %edx,%edx		# 如果函数指针为空
	jne 1f
	movl $_unexpected_hd_interrupt,%edx  # edx=_unexpected_hd_interrupt
1:	outb %al,$0x20	# 向8259A主芯片送EOI指令
	call *%edx		# "interesting" way of handling intr. 调用函数指针。
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

# int38 0x26， 软盘中断
_floppy_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	movb $0x20,%al
	outb %al,$0x20		# EOI to interrupt controller #1
	xorl %eax,%eax
	xchgl _do_floppy,%eax
	testl %eax,%eax
	jne 1f
	movl $_unexpected_floppy_interrupt,%eax
1:	call *%eax		# "interesting" way of handling intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

# int39 0x27 并行口中断函数。
_parallel_interrupt:
	pushl %eax
	movb $0x20,%al
	outb %al,$0x20
	popl %eax
	iret
