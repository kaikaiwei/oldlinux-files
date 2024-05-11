/*
 *  linux/boot/head.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  head.s contains the 32-bit startup code.
 *
 * NOTE!!! Startup happens at absolute address 0x00000000, which is also where
 * the page directory will exist. The startup code will be overwritten by
 * the page directory.
 * 第一：该程序会被打包到system的最开始部分，所以叫做head。
 * 从这里开始，内核完全在保护模式下运行。
 * 这里采用了AT&T的汇编语言格式。并且使用GNU的gas（后改名为as），gld（后改名为ld）进行编译连接。赋值方式从左到右。
 * 这段代码从地址0开始执行。主要功能如下：
 * 首先是加载各个数据段寄存器，重新设置中断描述符表int，共256项，并使各个表项均指向一个只报错误的哑中断处理程序。
 * 然后重新设置全局描述符表gdt。
 * 接着使用武力地址0与1MB开始处内容比较的方法，检测A20地址线是否真正开启。
 * 测试pc是否包含数学协处理器芯片（80287和80387，或其他兼容芯片），并在控制寄存器CR0中设置相应的标记位。
 * 接着设置管理内存的分野处理机制，将页目录表放在绝对武力地址0的开始处。紧随其后的是4个页表（可寻址16MB内存），设置页表的表项。
 * 最后利用返回指令将于预先放置在堆栈中的/init/main.c程序地址弹出，运行main程序。
 */
.text
.globl _idt,_gdt,_pg_dir,_tmp_floppy_area
_pg_dir:
startup_32:			#这里设置各个段寄存器
	movl $0x10,%eax	#eax=0x10。 
# 对于GNU来说，立即数要加上$符号，否则表示地址。每个寄存器都要以%开头。0x10是全局段描述符表中的偏移量。
# 0x10含义：特权为0，选择全局描述符表（位2为0），选择表中第二项（位3-15为2）。指向表中的数据段描述符项。
	mov %ax,%ds		# ds=0x10
	mov %ax,%es		# es=0x10
	mov %ax,%fs		# fs=0x10
	mov %ax,%gs
	lss _stack_start,%esp	# 表示设置系统堆栈。 _stack_start->ss:esp, 设置系统堆栈
	call setup_idt			# 调用setup_idt
	call setup_gdt			# 调用setup_gdt
	movl $0x10,%eax		# reload all the segment registers. 重新加载段寄存器
	mov %ax,%ds		# after changing gdt. CS was already	修改gdt后，cs代码寄存器已经在setup_gdt中重新加载过了
	mov %ax,%es		# reloaded in 'setup_gdt'
	mov %ax,%fs
	mov %ax,%gs
	lss _stack_start,%esp	# 表示设置系统堆栈。 _stack_start->ss:esp, 设置系统堆栈
	xorl %eax,%eax		# eax为0
1:	incl %eax		# check that A20 really IS enabled
	movl %eax,0x000000	# loop forever if it isn't
	cmpl %eax,0x100000	# 比较eax中数据与0x10000的数是否相等。如果相等就
	je 1b
/*
 * NOTE! 486 should set bit 16, to check for write-protect in supervisor
 * mode. Then it would be unnecessary with the "verify_area()"-calls.
 * 486 users probably want to set the NE (#5) bit also, so as to use
 * int 16 for math errors.
 * 下面开始检查数字协处理器。
 * 方法是修改控制寄存器cr0，如果存在数字协处理器，执行一个协处理器指令，否则表明不存在。
 * 设置cr0中的协处理器仿真位EM（位2）， 并复位协处理器存在标志MP(位1)
 */
	movl %cr0,%eax		# check math chip  
	andl $0x80000011,%eax	# Save PG,PE,ET
/* "orl $0x10020,%eax" here for 486 might be good */
	orl $2,%eax		# set MP
	movl %eax,%cr0
	call check_x87
	jmp after_page_tables

/*
 * We depend on ET to be correct. This checks for 287/387.
 * 我们依赖pe标志来检查80287和80387芯片
 */
check_x87:
	fninit
	fstsw %ax
	cmpb $0,%al
	je 1f			/* no coprocessor: have to set bits，如果存在则向前跳转到标号1出 */
	movl %cr0,%eax
	xorl $6,%eax		/* reset MP, set EM */
	movl %eax,%cr0
	ret
.align 2	# 4字节对齐。
1:	.byte 0xDB,0xE4		/* fsetpm for 287, ignored by 387。 287处理器码，会被387忽略掉 */
	ret

/*
 *  setup_idt
 *
 *  sets up a idt with 256 entries pointing to
 *  ignore_int, interrupt gates. It then loads
 *  idt. Everything that wants to install itself
 *  in the idt-table may do so themselves. Interrupts
 *  are enabled elsewhere, when we can be relatively
 *  sure everything is ok. This routine will be over-
 *  written by the page tables.
 *  设置中断描述符表。
 *  中断描述符表设置为256项，每项8Bytes，共2048字节。并且都指向哑中断程序。
 * 格式：Gate Descriptor，们表示符
 *  0-1B，6-7字节是偏移量
 *  2-3B是选择符
 *  4-5B一些标识位
 */
setup_idt:
	lea ignore_int,%edx		# edx的值为ignore_int的地址。
	movl $0x00080000,%eax	# eax=的高16为0x0008
	movw %dx,%ax		/* selector = 0x0008 = cs。 偏移值的低16为放置到ax中。现在eax表示门限定符的低4字节 */
	movw $0x8E00,%dx	/* interrupt gate - dpl=0, present  edx中包含门限定符的高4字节*/

	lea _idt,%edi		# _idt 是中断描述符表的地址。edi表示目的地址。
	mov $256,%ecx		# 循环次数。
rp_sidt:				# 循环进行赋值
	movl %eax,(%edi)	# eax的高16位放入到edi中，表示中断程序的地址
	movl %edx,4(%edi)	# edx的高16位放入到edi指向地址偏移4字节的地址中。
	addl $8,%edi		# edi+8，表示下一个表项的地址
	dec %ecx			# ecx-=1，减少循环次数。
	jne rp_sidt			# exc不为0，继续循环
	lidt idt_descr		# 加载中断描述符表寄存器的值
	ret

/*
 *  setup_gdt
 *
 *  This routines sets up a new gdt and loads it.
 *  Only two entries are currently built, the same
 *  ones that were built in init.s. The routine
 *  is VERY complicated at two whole lines, so this
 *  rather long comment is certainly needed :-).
 *  This routine will beoverwritten by the page tables.
 *  设置全局描述符表
 *  设置一个新的gdt，并加载。
 *  此时仅创建两个表项。
 */
setup_gdt:
	lgdt gdt_descr # 加载全局描述符表寄存器。在后面定义。
	ret

/*
 * I put the kernel page tables right after the page directory,
 * using 4 of them to span 16 Mb of physical memory. People with
 * more than 16MB will have to expand this.
 */
.org 0x1000  # 定义下面的内存数据块从0x1000开始
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000	# 定义下面的内存数据块从0x5000开始
/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isn't
 * on a 64kB border.
 * 软盘驱动，当dma不能访问缓冲块时，下面tmp_floppy_area内存块就可供软盘驱动器使用。
 * 地址需要对齐调整，这样就不会跨越64kB的边界。
 */
_tmp_floppy_area:
	.fill 1024,1,0	# 共保留1024项，每项1字节，填充数字0

/**
 * 入站操作是为了调用init/main.c程序和返回做准备的。
 */
after_page_tables:
	pushl $0		# These are the parameters to main :-) 这些事调用main程序的参数。 path或env
	pushl $0		# argc
	pushl $0		# argc
	pushl $L6		# return address for main, if it decides to. 返回地址，如果main.c真的退出了，就会到这里。
	pushl $_main	# _main是编译程序对main的内部表示方法
	jmp setup_paging	# 跳转到setup_paging， 进行分页
L6:
	jmp L6			# main should never return here, but   正常情况下不会到这里。只是为了以防万一。
				# just in case, we know what happens.

/* This is the default interrupt "handler" :-)
 * 这是默认的中断处理程序
 */
int_msg:
	.asciz "Unknown interrupt\n\r"	# 定义字符串，未知中断
.align 2			# 按照4字节方式对齐内存地址
ignore_int:			# 中断向量具柄
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds		# ds，es，fs，gs等，虽然是16位寄存器，但入站后仍然会以32位的形式入栈，占用4个字节的空间
	push %es
	push %fs
	movl $0x10,%eax	# 将段选择符指向gdt表中的数据段。eax的高位被置为0x10
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	pushl $int_msg	# 把调用_printk函数的参数指针地址入栈。
	call _printk	# 调用_printk函数，时printk编译后模块中的内部表示法
	popl %eax		# 函数返回结果。
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret			# 中断返回，把中断调用时压入栈的cpu标志寄存器（32位）值页弹出。


/*
 * Setup_paging
 *
 * This routine sets up paging by setting the page bit
 * in cr0. The page tables are set up, identity-mapping
 * the first 16MB. The pager assumes that no illegal
 * addresses are produced (ie >4Mb on a 4Mb machine).
 *
 * NOTE! Although all physical memory should be identity
 * mapped by this routine, only the kernel page functions
 * use the >1Mb addresses directly. All "normal" functions
 * use just the lower 1Mb, or the local data space, which
 * will be mapped to some other place - mm keeps track of
 * that.
 *
 * For those with more memory than 16 Mb - tough luck. I've
 * not got it, why should you :-) The source is here. Change
 * it. (Seriously - it shouldn't be too difficult. Mostly
 * change some constants etc. I left it at 16Mb, as my machine
 * even cannot be extended past that (ok, but it was cheap :-)
 * I've tried to show which constants to change by having
 * some kind of marker at them (search for "16Mb"), but I
 * won't guarantee that's all :-( )
 * 
 * 开始分页。 通过设置控制寄存器cr0的盘标志（PG位，31）来启动对内存的分页处理功能。并设置各个页表项的内容，以恒等映射前16MB的物理地址。
 * 分页器假定不会产生非法地址映射（例如：在只有4MB的映射出大于4MB的内存地址）。
 * 只有内核页面管理函数能直接使用大于1MB的地址。
 * 一般函数使用低于1MB的地址空间，或者使用局部数据空间，地址空间将映射到其他地方上去。由mm（内存管理程序）来进行管理。
 */
.align 2		# 按照4字节对齐方式
setup_paging:	# 首先对5页内存（1页目录+4页页表）清零
	movl $1024*5,%ecx		/* 5 pages - pg_dir+4 page tables ， ecx=1024*5，表示4KB*5，20KB的数据*/
	xorl %eax,%eax			/* eax=0*/
	xorl %edi,%edi			/* pg_dir is at 0x000， edi，目标地址为0 */
	cld;rep;stosl			/* cld表示的是增加方向，rep表示重复执行。stosl表示将eax的值，保存在es:edi中*/
/* 下面4句设置页目录中的项。只有4个页表，只需要设置4次。*/
/**
 * 页目录项的结果和页表中项的结构一样。4字节为1项。
 * $pg0+7 表示0x00001007，是页目录表中第一项。
 * 第一项的页表所在地址为   0x00001007 & 0xfffff000 = 0x1000 
 * 第一项的属性标志为      0x00001007 & 0x00000fff = 0x0007，表示该页存在，用户可读写。
 */
	movl $pg0+7,_pg_dir		/* set present bit/user r/w */
	movl $pg1+7,_pg_dir+4		/*  --------- " " --------- */
	movl $pg2+7,_pg_dir+8		/*  --------- " " --------- */
	movl $pg3+7,_pg_dir+12		/*  --------- " " --------- */
/** 
 * 下面6行填写4个页表中所有项的内容。共有4页表*1024（项）=4096项。也就是0x0-0xfff。也就是能映射物理地址4096*4KB=16MB的地址。
 * 每项的内容是：当前项所映射的物理内存地址+该页的标志（都是7）。
 * 使用最后一个页表的最后一项开始倒退顺序填写。
 */
	movl $pg3+4092,%edi		/* 目的地址，最后一个表项的地址*/
	movl $0xfff007,%eax		/*  16Mb - 4096 + 7 (r/w user,p) */
	std				# 方向位置位，edi值递减（4B）
1:	stosl			/* fill pages backwards - more efficient :-)。eax的值填充es：edi */
	subl $0x1000,%eax	# 每填写好一项eax将减去0x1000，就是就该物理地址
	jge 1b				/*如果小于0，表示全部填写完毕*/
/*下面设置页目录机制寄存器cr3的值，指向页目录表*/
	xorl %eax,%eax		/* pg_dir is at 0x0000， eax位0x0000*/
	movl %eax,%cr3		/* cr3 - page directory start */
	movl %cr0,%eax		/*设置启动分页处理，cr0的PG标志，位31*/
	orl $0x80000000,%eax	/*添加上pg标志*/
	movl %eax,%cr0		/* set paging (PG) bit，设置pg标志位 */
	ret			/* this also flushes prefetch-queue */
/* 在改变分页处理标志后，要求使用转移指令刷新预取指令队列，这里使用的返回指令是ret。
 * 该返回指令的另一个作用是将堆栈中的main程序地址弹出，并开始运行/init/main.c程序，本程序到此结束。
 */

.align 2		# 4字节对齐
.word 0			# 
idt_descr:		# 下面两行是lidt指令的6B操作数。长度和基址
	.word 256*8-1		# idt contains 256 entries
	.long _idt
.align 2
.word 0
gdt_descr:				# 全局描述符表。lgdt的6B操作数，长度和基址
	.word 256*8-1		# so does gdt (not that that's any
	.long _gdt		# magic number, but it works for me :^)

	.align 3		#按照8字节方式对齐内存地址边界
_idt:	.fill 256,8,0		# idt is uninitialized。 256项，每项8字节，填充0

/*
 * 全局表
 * 前四项分别为：空项，代码段描述符，数据段描述符，系统段描述符。其中，系统段描述符暂时没有使用。
 * 后main预留了252个空间，用于存放所创建任务的局部描述符（LDT）和对应的任务状态段（TSS）的描述符。
 */
_gdt:	.quad 0x0000000000000000	/* NULL descriptor  第一项位空。*/
	.quad 0x00c09a0000000fff	/* 16Mb  代码段最大长度16MB*/
	.quad 0x00c0920000000fff	/* 16Mb  数据段最大长度16MB*/
	.quad 0x0000000000000000	/* TEMPORARY - don't use */
	.fill 252,8,0			/* space for LDT's and TSS's etc */
