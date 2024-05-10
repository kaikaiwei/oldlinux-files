!
! SYS_SIZE is the number of clicks (16 bytes) to be loaded.
! 0x3000 is 0x30000 bytes = 196kB, more than enough for current
! versions of linux
! 这个是系统大小。 编译连接后的系统模块的大小
SYSSIZE = 0x3000
!
!	bootsect.s		(C) 1991 Linus Torvalds
!
! bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
! iself out of the way to address 0x90000, and jumps there.
!
! It then loads 'setup' directly after itself (0x90200), and the system
! at 0x10000, using BIOS interrupts. 
!
! NOTE! currently system is at most 8*65536 bytes long. This should be no
! problem, even in the future. I want to keep it simple. This 512 kB
! kernel size should be enough, especially as this doesn't contain the
! buffer cache as in minix
!
! The loader has been made as simple as possible, and continuos
! read errors will result in a unbreakable loop. Reboot by hand. It
! loads pretty fast by getting whole sectors at a time whenever possible.

! 定义了六个全局标识
.globl begtext, begdata, begbss, endtext, enddata, endbss
.text begin    ! 文本段
begtext:
.data			! 数据段
begdata:
.bss			! 未初始化数据段 Block started by Symbol
begbss:
.text

SETUPLEN = 4				! nr of setup-sectors  初始化程序扇区
BOOTSEG  = 0x07c0			! original address of boot-sector  加电自举地址
INITSEG  = 0x9000			! we move boot here - out of the way 启动后移动到的位置
SETUPSEG = 0x9020			! setup starts here						setup城区的初始化位置
SYSSEG   = 0x1000			! system loaded at 0x10000 (65536).		系统加载位置。
ENDSEG   = SYSSEG + SYSSIZE		! where to stop loading				系统结束位置。

! ROOT_DEV:	0x000 - same type of floppy as boot.  根文件系统使用软盘引导
!		0x301 - first partition on first drive etc  第一块硬盘的第一个分区
ROOT_DEV = 0x306    ! 根文件系统在第二块硬盘的第一个分区。  硬盘的主设备号是3。

！ 告知链接器，从start的位置开始执行
entry start
！ 将地址从0x7c0转换到0x9000
start:
	mov	ax,#BOOTSEG		！ ax为0x07c0
	mov	ds,ax			！ ds为0x07c0。ds是段寄存器
	mov	ax,#INITSEG		！ ax为0x9000
	mov	es,ax			！ es为0x9000。es是段寄存器
	mov	cx,#256			！ cx为256， 是移动计数
	sub	si,si			！ si为0，源地址：ds：si=0x07c0：0x0000
	sub	di,di			！ di为0，源地址：es：di=0x9000：0x0000
	rep					！ 重复执行，直到cx为0
	movw				！ 移动一个字。
	jmpi	go,INITSEG	！ 段间跳转。就是将cs的值更新为INITSEG，也就是0x9000
！ cpu已经跳转到0x9000的位置了
go:	mov	ax,cs
	mov	ds,ax
	mov	es,ax
! put stack at 0x9ff00.  将堆栈指针sp设置为0x90000，ss是堆栈段寄存器。cs是代码段寄存器
！sp的值就是0x9000：0xFF00
	mov	ss,ax
	mov	sp,#0xFF00		! arbitrary value >>512

! load the setup-sectors directly after the bootblock. 在bootblock后，加载setup模块
! Note that 'es' is already set up.

load_setup:
	mov	dx,#0x0000		! drive 0, head 0  0号驱动器，0号磁头
	mov	cx,#0x0002		! sector 2, track 0 第二个扇区，磁到0
	mov	bx,#0x0200		! address = 512, in INITSEG   位置。INITSEG段，512偏移处。
	mov	ax,#0x0200+SETUPLEN	! service 2, nr of sectors  服务号2，后面是扇区数
	int	0x13			! read it    读取数据。以中断吗？
	jnc	ok_load_setup		! ok - continue 正常情况下，继续处理。
	mov	dx,#0x0000		！将dx设置为0x0000
	mov	ax,#0x0000		! reset the diskette ，将ax设置为0x0000。 复位磁盘
	int	0x13			！读取数据
	j	load_setup		！跳转到load_setup

# 加载完成后
ok_load_setup:
# 取磁盘驱动器参数。特别是没到扇区数量。
! Get disk drive parameters, specifically nr of sectors/track

	mov	dl,#0x00		！将dl设置为0x00。表示驱动器数量。
	mov	ax,#0x0800		! AH=8 is get drive parameters 设置参数。
	int	0x13			! 磁盘读取数据
	mov	ch,#0x00		！
	seg cs				！表示下一条语句的操作数在cs段寄存器所指的段中。应该是硬盘将数据读取到了cs中。
	mov	sectors,cx		！保存每磁道扇区数
	mov	ax,#INITSEG		！将ax设置为0x9000
	mov	es,ax			！es为0x9000。表示额外的段寄存器

! Print some inane message

	mov	ah,#0x03		! read cursor pos 读取光标位置，ah=0x03
	xor	bh,bh			！抑或bh=bh^bh， 也就是bh=0
	int	0x10			！发出10号中断。
	
	mov	cx,#24			！计数器，表示共计读取24个字符
	mov	bx,#0x0007		! page 0, attribute 7 (normal)
	mov	bp,#msg1
	mov	ax,#0x1301		! write string, move cursor 写字符，并移动光标
	int	0x10			！发出10号中断

! ok, we've written the message, now
! we want to load the system (at 0x10000) 
！将system模块加载到64kb处。

	mov	ax,#SYSSEG
	mov	es,ax		! segment of 0x010000  es为system段地址
	call	read_it		！读取磁盘上system模块，es为输入参数
	call	kill_motor  ！关闭电动机，这样就可以知道驱动器的状态了。

! After that we check which root-device to use. If the device is
! defined (!= 0), nothing is done and the given device is used.
! Otherwise, either /dev/PS0 (2,28) or /dev/at0 (2,8), depending
! on the number of sectors that the BIOS reports currently.
！ 加载完成后，检查要使用哪个根文件系统设备。如果指定了，就使用对应的设备。
！ 否则根据bios报告的每磁道扇区数来确定使用/dev/PS0还是/dev/at0。
	seg cs				！表示下一条语句的操作数在cs段寄存器所指的段中。
	mov	ax,root_dev 	！ 取508开始的2个字节的根设备号
	cmp	ax,#0			！ ax 是否为0
	jne	root_defined	！ 如果不为0，表示拥护已经指定根文件系统设备。
	seg cs				！表示下一条语句的操作数在cs段寄存器所指的段中。	
	mov	bx,sectors		！bx为每磁道扇区数，本文94行保存的。
	mov	ax,#0x0208		! /dev/ps0 - 1.2Mb 。 
	cmp	bx,#15			！判断每磁道扇区数，如果等于15，那么就跳转到root_defined中执行。
	je	root_defined
	mov	ax,#0x021c		! /dev/PS0 - 1.44Mb
	cmp	bx,#18			！判断每磁道扇区数，如果等于18，那么就跳转到root_defined中执行。
	je	root_defined
undef_root:				！异常情况，则死循环。
	jmp undef_root
root_defined:
	seg cs
	mov	root_dev,ax		！设置根文件系统所在的设备号

! after that (everyting loaded), we jump to
! the setup-routine loaded directly after
! the bootblock:
！ 到这里后，所以程序都加载完毕。跳转到setup程序中去（setup程序在在0x9020：0000中，载bootsect后面）

	jmpi	0,SETUPSEG

! This routine loads the system at address 0x10000, making sure
! no 64kB boundaries are crossed. We try to load it as fast as
! possible, loading whole tracks whenever we can.
!
! in:	es - starting address segment (normally 0x1000)
!  
！ 这个字程序将系统模块加载道地址0x10000处，由es指定。 并求额定没有跨越64KB的内存便捷。
sread:	.word 1+SETUPLEN	! sectors read of current track  磁盘中已经读取的扇区数。开始时已读如1扇区的引导扇区
head:	.word 0			! current head  当前磁头号
track:	.word 0			! current track 当前磁道号

read_it:
	mov ax,es       ！ax为0x10000，将es赋值给ax。
	test ax,#0x0fff ！ax & 0x0fff。 将低位给置0
！ Test对两个参数(目标，源)执行AND逻辑操作，并根据结果设置标志寄存器，结果本身不会保存。影响标志：C,O,P,Z,S(其中C与O两个标志会被设为0)
！ jne是一个条件转移指令，当ZF=0，转至标号处执行，格式是JNE/JNZ标号。
die:	jne die			! es must be at 64kB boundary， es的值必须为64kb的边界。 否则进入死循环
	xor bx,bx		! bx is starting address within segment bx为段内偏移量。将其置为0
rp_read:
	mov ax,es		！ 将ex赋值给ax。
	cmp ax,#ENDSEG		! have we loaded all yet? 是否已经加载所有的数据。ENDSEG   = SYSSEG + SYSSIZE，系统结束位置。SYSSIZE = 0x3000，SYSSEG=0x1000
	jb ok1_read
	ret				！子程序返回指令

！ 计算和验证但前磁道需要读取的扇区数，放在ax寄存器中。
！ 根据但前磁道还未读取的扇区数以及段内数据字节开始偏移位置，计算如果读取全部这些未读扇区，所读总字节是否超过64KB段长限制。
！ 如果超过，则根据此次最多能读取的字节数（64KB-段内偏移量），酸楚此次需要读取的扇区数
ok1_read:
	seg cs
	mov ax,sectors		！ax为每磁道扇区数
	sub ax,sread		！ax减去已经读取到的扇区数
	mov cx,ax			！cx为需要读取的扇区数
	shl cx,#9			！cx*512字节
	add cx,bx			！cx=cx+bx， bx表示段内偏移，也就是说，cx为此次操作后，段内读如的字节数
	jnc ok2_read		！如果没有超过64KB，那就跳转到ok2_read
	je ok2_read			！
	xor ax,ax			！
	sub ax,bx			！ax=ax-bx, 减去段内偏移. 计算这次最多能读入的字节数（64KB-段内偏移量）
	shr ax,#9			！ax=ax/512。 转换为需要读取的扇区数
ok2_read:
	call read_track		！调用read_track子函数
	mov cx,ax			！cx=ax。 本次操作已经读取的扇区数。
	add ax,sread		！ax为当前磁道上已经读取的扇区数
	seg cs
	cmp ax,sectors		！如果当前磁道上还有扇区未读，则跳转到ok3_read中
	jne ok3_read		！跳转到ok3_read
	mov ax,#1			！读1号磁头上的数据
	sub ax,head			！判断当前磁头号。ax-当前磁头号。0号磁头和1号磁头。
	jne ok4_read		！如果是0号磁头，则再去读1号磁头面上的扇区数据
	inc track			！当前磁道+1
！读取磁道，
ok4_read:
	mov head,ax			！保存当前磁头号
	xor ax,ax			！清当前磁道已读扇区数
！磁道中读取数据
ok3_read:
	mov sread,ax		！保存当前磁道已读扇区数
	shl cx,#9			！cx为已经读取的扇区数*512
	add bx,cx			！bx =bx+cx。调整当前段内数据开始位置
	jnc rp_read			！如果小于64KB，则跳转到rp_read。
	mov ax,es			！ax=es，0x1000 
	add ax,#0x1000		！ax=ax+0x1000， 调整到下一个64KB的位置
	mov es,ax			！es=ax， 更新段地址
	xor bx,bx			！bx=0， 将段内偏移清空
	jmp rp_read			！跳转到rp_read

！读取磁道
read_track:
	push ax
	push bx
	push cx
	push dx
	mov dx,track		！dx为当前磁道号
	mov cx,sread		！cx为当前磁道上已经读取的扇区数
	inc cx				！cx=cx+1
	mov ch,dl			！ch=dl。
	mov dx,head			！dx为当前磁道号。
	mov dh,dl			！
	mov dl,#0			！
	and dx,#0x0100		！磁头号不大于1
	mov ah,#2			！ah=2， 读磁盘扇区功能号
	int 0x13			！ 13号中断。
	jc bad_rt			! 如果出错，跳转到bad_rt中
	pop dx				！出栈操作
	pop cx
	pop bx
	pop ax
	ret					！函数返回
bad_rt:	mov ax,#0		！ax为0，执行驱动器复位操作（磁盘中断号为0），内在跳转到read_track中重试
	mov dx,#0			！dx为0，表示
	int 0x13			！13号磁盘
	pop dx				！出栈操作
	pop cx	
	pop bx
	pop ax
	jmp read_track		！跳转到read_track中执行

/*
 * This procedure turns off the floppy drive motor, so
 * that we enter the kernel in a known state, and
 * don't have to worry about it later.
 * 关闭软盘驱动电机。这样，载进入内核后，它就处于已知状态
 */
kill_motor:
	push dx
	mov dx,#0x3f2	！软驱控制卡的驱动端口，只写
	mov al,#0		！A去东区，关闭fdc，禁止dma和中断请求，关闭电机
	outb			！将al的内容输出到dx指定的端口。是在io.h中定义的宏吗。
	pop dx
	ret

！存放当前启动软盘每磁道的扇区数
sectors:
	.word 0

！系统加载的输出信息，一般作为自定义加载的初始位置。
msg1:
	.byte 13,10						！回车换行的ascii码
	.ascii "Loading system ..."
	.byte 13,10,13,10				！回车换行的ascii码

.org 508		！表示后面语句从地址508（0x1pc）开始，所以root_dev在启动扇区第508开始的2B中
root_dev:
	.word ROOT_DEV		！存放跟文件系统所在的设备号（init.main.c中会用到）
boot_flag:
	.word 0xAA55	！硬盘有效标识

.text
endtext:
.data
enddata:
.bss
endbss:
