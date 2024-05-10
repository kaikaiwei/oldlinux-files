!
!	setup.s		(C) 1991 Linus Torvalds
!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! This code asks the bios for memory/disk/other parameters, and
! puts them in a "safe" place: 0x90000-0x901FF, ie where the
! boot-block used to be. It is then up to the protected mode
! system to read them from there before the area is overwritten
! for buffer-blocks.
!

！ 目的：使用biso中断读取机器系统数据。并将这些数据保存在0x90000中，覆盖掉bootsect所在的位置。
! NOTE! These had better be the same as in bootsect.s!

INITSEG  = 0x9000	! we move boot here - out of the way 原来bootsec所处的段
SYSSEG   = 0x1000	! system loaded at 0x10000 (65536).  system镜像在0x10000（64KB）处。
SETUPSEG = 0x9020	! this is the current segment		 setup程序，也就是本程序的加载位置

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text		！代码段
begtext:
.data		！数据段
begdata:
.bss		！未初始化数据段。block section size
begbss:
.text

！程序进入点
entry start
start:

! ok, the read went well so we get current cursor position and save it for
! posterity.
！整个磁盘的读取过程都征程，现在将光标位置保存起来。

	mov	ax,#INITSEG	! this is done in bootsect already, but...  将ax设置为0x9000
	mov	ds,ax		！ds为0x9000
！bios中断0x10的读光标功能号ah=0x03. bh为页号。返回ch=扫描开始线，cl=扫描结束线
！dh=行号（0x00是顶端），dl是列号（0x00是左边）
！dx 寄存器里的值表示光标的位置，具体说来其高八位 dh 存储了行号，低八位 dl 存储了列号。
	mov	ah,#0x03	! read cursor pos。 
	xor	bh,bh		！bh=0，页号为0
	int	0x10		! save it in known place, con_init fetches  发起中断。
	mov	[0],dx		! it from 0x90000. 这个内存地址仅仅是偏移地址，还需要加上 ds 这个寄存器里存储的段基址，最终的内存地址是在 0x90000 处
！将光标位置放在0x9000处，控制台初始化的时候从这里获取（con_init函数）。这里放置的是2个字节


! Get memory size (extended mem, kB) 获取内存信息

	mov	ah,#0x88
	int	0x15
	mov	[2],ax   ！内存信息存放0x90002。在这个内存地址仅仅是偏移地址，还需要加上 ds 这个寄存器里存储的段基址，最终的内存地址是在 0x90000 处

! Get video-card data: 获取显卡显示模式

	mov	ah,#0x0f
	int	0x10
	mov	[4],bx		! bh = display page		0x90002	，存放显示页面
	mov	[6],ax		! al = video mode, ah = window width  0x90006，存放显示模式。0x90007，存放字符列数

! check for EGA/VGA and some config parameters 检查显示方式并获取参数

	mov	ah,#0x12
	mov	bl,#0x10
	int	0x10
	mov	[8],ax		！0x90008
	mov	[10],bx		！0x9000a 显示内存，0x00 64KB，0x01 128KB，0x10 192KB，0x11 256KB. 0x9000b 显示状态， 0x00 彩色 I/O=0x3dX， 0x11 单色，0x3bX。 
	mov	[12],cx		！0x9000c 特性参数，显卡的特性参数。2字节。

! Get hd0 data  获取第一块硬盘的信息。

	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x41]		!取中断向量0x41的值，基hd0参数表的地址 ds:si 
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0080		!传输的目的地。0x9000:0x0080  es:di 
	mov	cx,#0x10		!总共传输10个字节
	rep
	movsb

! Get hd1 data  获取第二块硬盘的值。参见上面

	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x46]
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10
	rep
	movsb

! Check that there IS a hd1 :-)  检查是否存在hd1，也就是第二块磁盘。不能存在

	mov	ax,#0x01500 ！功能号
	mov	dl,#0x81	！驱动器号， 0x81是第二个硬盘，0x80是第一个硬盘
	int	0x13		！ 获取磁盘类型
	jc	no_disk1	！
	cmp	ah,#3		！类型为3， 表示为硬盘。
	je	is_disk1
no_disk1:			！第二块硬盘不存在，需要对第二个硬盘表进行清零操作
	mov	ax,#INITSEG	
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10
	mov	ax,#0x00
	rep
	stosb
is_disk1:

! now we want to move to protected mode ...  准备进入保护模式

	cli			! no interrupts allowed !  关中断

! first we move the system to it's rightful place  移动系统镜像到正确的地点。
！bootsec引导程序是将system模块读入到0x10000（64kB）开始的地方。
！当时假设system模块最大长度不会超过0x80000（512KB），也就是末端地址是0x90000.
！所以bootsec才会将自己移动到0x90000开始的地方，并把setup加载道它的后面。
！下面是把system模块移动到0x00000位置，也就是把[0x10000,0x8ffff]（512KB），整块的想内存地段地址移动。
	mov	ax,#0x0000		！ax=0x0000，也就是系统镜像的新地址
	cld			! 'direction'=0, movs moves forward 设置方向为0。
do_move:
	mov	es,ax		! destination segment  es:di -> 目的地址，初始化为0x0000:0x0
	add	ax,#0x1000	！ax+=0x1000， 也就是说。ax=0x1000
	cmp	ax,#0x9000  ! 比较ax和0x9000， 
	jz	end_move	！如果ax为0x9000，那么就跳转到end_move中执行
	mov	ds,ax		! source segment    ds=ax， 保存ax  ds:si -> 源地址，初始化为：0x9000:0x0
	sub	di,di		！di = 0
	sub	si,si		！si = 0
	mov 	cx,#0x8000		！cx为0x8000， 表示需要移动多少个字
	rep				！ 循环执行
	movsw			！ 移动一个字
	jmp	do_move		！跳转到do_move，重复执行本段代码，也就是所谓的递归执行。

! then we load the segment descriptors 此后，我们加载段描述符号。
！下面由32位保护模式的操作。
！lidt指令用于加载中断描述附表（idt）寄存器。它的操作数是6个字节，0-1描述符表的长度。2-5表示32位线性地址（收地址）。
！中断描述符表的每个表项（8字节）指出发生中断时需要调用的代码的信息。
！lgdt指令用于加载全局描述符表（gdt）寄存器，其操作数与idt西安痛。
！全局描述符表中每个描述符项（8字节，8B）描述了保护模式下数据和代码段的信息，包括段的最大长度限制（16位），段的线性地址（32位）
！段的特权级，段是否载内存，读写许可以及一些包膜模式运行的标志。

end_move:
	mov	ax,#SETUPSEG	! right, forgot this at first. didn't work :-)   ax设置为SETUPSEG，也就是0x9020
	mov	ds,ax			！ dx = ax = 0x9020
	lidt	idt_48		! load idt with 0,0。 lidt指令用于加载中断描述符表（idt）寄存器。详见本文最后的结构体定义
	lgdt	gdt_48		! load gdt with whatever appropriate    加载全局中断描述符表寄存器

! that was painless, now we enable A20  上面的操作很简单，现在我们要开启A20地址线。

	call	empty_8042	！等待输入缓存器空。只有当输入缓冲器为空时，才能对起写命令。
	mov	al,#0xD1		! command write  ax的低位写入0xD1， 0xD1是命令码
	out	#0x64,al		！ 8042的 p2 口。 p2口的位1用于A20线选通，数据要写入到0x60口。
	call	empty_8042	！
	mov	al,#0xDF		! A20 on   选通A20口的参数
	out	#0x60,al		！ 8042的 p2 口。 p2口的位1用于A20线选通，数据要写入到0x60口。
	call	empty_8042  ！ 输入缓冲期位空，表示A20线已经选通

! well, that went ok, I hope. Now we have to reprogram the interrupts :-(
! we put them right after the intel-reserved hardware interrupts, at
! int 0x20-0x2F. There they won't mess up anything. Sadly IBM really
! messed this up with the original PC, and they haven't been able to
! rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,
! which is used for the internal hardware interrupts as well. We just
! have to reprogram the 8259's, and it isn't fun.
！ 下面是对中断进行编程，将其放在intel保留的中断后面。在int 0x20-0x2F之间。
！ 

	mov	al,#0x11		! initialization sequence  初始化序列。al为0x11
	out	#0x20,al		! send it to 8259A-1      将0x11 发送给8059A-1芯片。
	.word	0x00eb,0x00eb		! jmp $+2, jmp $+2  延迟等待指令。跳转到下一个指令，执行两次。
	out	#0xA0,al		! and to 8259A-2  将0x11 发送给8059A-2芯片。
	.word	0x00eb,0x00eb
	mov	al,#0x20		! start of hardware int's (0x20)   al设置为0x20， 开始20号中断？
	out	#0x21,al		！送主芯片icw2命令字，起始中断号，要送奇地址
	.word	0x00eb,0x00eb
	mov	al,#0x28		! start of hardware int's 2 (0x28)
	out	#0xA1,al		！送从芯片icw2命令字，起始中断号，要送奇地址
	.word	0x00eb,0x00eb
	mov	al,#0x04		! 8259-1 is master	al=0x04， 表示为主芯片
	out	#0x21,al		！ 将0x21。送主芯片icw3命令字，表示主芯片的ir2连接到从芯片的ir。
	.word	0x00eb,0x00eb
	mov	al,#0x02		! 8259-2 is slave  al=0x02，表示从芯片
	out	#0xA1,al		！ 送从芯片icw4命令字，表示从芯片int连接到主芯片ir2引脚
	.word	0x00eb,0x00eb
	mov	al,#0x01		! 8086 mode for both	
	out	#0x21,al		！送主芯片icw4命令字。8086模式，普通boi模式
	.word	0x00eb,0x00eb
	out	#0xA1,al		！送从芯片icw4命令字。8086模式，普通boi模式
	.word	0x00eb,0x00eb
	mov	al,#0xFF		! mask off all interrupts for now 屏蔽所有中断
	out	#0x21,al		！屏蔽主芯片所有中断请求
	.word	0x00eb,0x00eb	
	out	#0xA1,al		！屏蔽从芯片所有中断请求

! well, that certainly wasn't fun :-(. Hopefully it works, and we don't
! need no steenking BIOS anyway (except for the initial loading :-).
! The BIOS-routine wants lots of unnecessary data, and it's less
! "interesting" anyway. This is how REAL programmers do it.
!
! Well, now's the time to actually move into protected mode. To make
! things as simple as possible, we do no register set-up or anything,
! we let the gnu-compiled 32-bit programs do that. We just jump to
! absolute address 0x00000, in 32-bit protected mode.

	mov	ax,#0x0001	! protected mode (PE) bit   32位保护模式比特为（PE)
	lmsw	ax		! This is it!		就这样加载加载机器状态字
	jmpi	0,8		! jmp offset 0 of segment 8 (cs)	跳主动拿到cs（段8），偏移0处。
！这里的段值8已经是保护模式下的段选择符了。用于选择描述符表，描述符表项，和特权集。
！段选择符长度为16位（2字节），位0-1表示特权级别（0-3），linux只用到两级。位3-15送描述符表的索引，指出选择第几项描述符。
！所以段选择符8（0x0000，0000，0000，0100）表示请求特权级别0，使用全局描述符表的第一项。
！该项指出代码的基地址是0，因此就跳转到system的代码中去了。
！system的结构中，分别是head.s, main.c程序，内核模块，内存管理模块，库模块。。。然后就是系统参数（0x90000处）。
！所以jmpi 0，8 会跳转到head.s中执行。



! This routine checks that the keyboard command queue is empty
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldn't proceed anyway.
！ 循环检查键盘命令队列是否为空。这里不使用超时方法，如果这里死机，表示pc由问题。
！只有当输入缓冲器为空（状态寄存器2=0）才可以对起进行写命令
empty_8042:
	.word	0x00eb,0x00eb	！连个跳转指令机器码，跳转到写一句，作为延迟空操作。
	in	al,#0x64	! 8042 status port   AT键盘控制状态寄存器
	test	al,#2		! is input buffer full?		
	jnz	empty_8042	! yes - loop   如果不为0，那么就loop，继续执行。 否则，通过ret指令返回。
	ret

！全局描述符表。由多个8字节长的描述符项组成。
！第一项不使用。235行，但是存在。
！第二个是系统代码段描述符（240-243）
！第三个是系统数据段描数据（245-249）
gdt:
	.word	0,0,0,0		! dummy  这里不使用，但是存在。

！这里在gdt的偏移量是0x08，当加载代码段寄存器（段选择符）时，使用的是这个偏移量
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9A00		! code read/exec
	.word	0x00C0		! granularity=4096, 386

！这里在gdt的偏移量是0x10，当加载数据段寄存器（如ds等）时，使用的是这个偏移量
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9200		! data read/write
	.word	0x00C0		! granularity=4096, 386

！这里是lidt指令的操作数。前两个字节表示表限长，后面4个字节表示idt所处的基地址
idt_48:
	.word	0			! idt limit=0
	.word	0,0			! idt base=0L

！这里是lgdt指令的操作数
gdt_48:
	.word	0x800		! gdt limit=2048, 256 GDT entries。表限长2048，每个8字节，所以共有256个表项
	.word	512+gdt,0x9	! gdt base = 0X9xxxx  
！ 计算方式，4字节构成的线性地址0x0009<<16 + 0x200(512) +gdt    yejiushi 0x90200 + gdt(gdt在这里表示在本程序中的偏移地址) 
	
.text
endtext:
.data
enddata:
.bss
endbss:
