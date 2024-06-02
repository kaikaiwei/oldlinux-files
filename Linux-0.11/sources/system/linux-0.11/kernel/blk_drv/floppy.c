/*
 *  linux/kernel/floppy.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 02.12.91 - Changed to static variables to indicate need for reset
 * and recalibrate. This makes some things easier (output_byte reset
 * checking etc), and means less interrupt jumping in case of errors,
 * so the code is hopefully easier to understand.
 */

/*
 * This file is certainly a mess. I've tried my best to get it working,
 * but I don't like programming floppies, and I have only one anyway.
 * Urgel. I should check for more errors, and do more graceful error
 * recovery. Seems there are problems with several drives. I've tried to
 * correct them. No promises. 
 */

/*
 * As with hd.c, all routines within this file can (and will) be called
 * by interrupts, so extreme caution is needed. A hardware interrupt
 * handler may not sleep, or a kernel panic will happen. Thus I cannot
 * call "floppy-on" directly, but have to set a special timer interrupt
 * etc.
 *
 * Also, I'm not certain this works on more than 1 floppy. Bugs may
 * abund.
 */

 /**
  * 软盘驱动器控制函数。
  */

#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#define MAJOR_NR 2
#include "blk.h"

static int recalibrate = 0;		// 重新校正标记
static int reset = 0;			// 重置标记
static int seek = 0;			// 寻道标记

extern unsigned char current_DOR;	// 当前数据输出寄存器 Digital Output Register

// 字节直接输出。 val：值。 port：端口。
#define immoutb_p(val,port) \
__asm__("outb %0,%1\n\tjmp 1f\n1:\tjmp 1f\n1:"::"a" ((char) (val)),"i" (port))

// 软驱类型： 2 ： 1.2MB   7： 1.4MB
#define TYPE(x) ((x)>>2)
// 软许序号。 0-3对应A-D
#define DRIVE(x) ((x)&0x03)
/*
 * Note that MAX_ERRORS=8 doesn't imply that we retry every bad read
 * max 8 times - some types of errors increase the errorcount by 2,
 * so we might actually retry only 5-6 times before giving up.
 * 最大请求次数。有些操作会将错误类型*2使用
 */
#define MAX_ERRORS 8

/*
 * globals used by 'result()'
 * result 函数使用的全局变量
 */
 // FDC 最多返回7个结果
#define MAX_REPLIES 7
// 存放FDC返回结果的信息
static unsigned char reply_buffer[MAX_REPLIES];
// FDC 便捷获取函数
#define ST0 (reply_buffer[0])
#define ST1 (reply_buffer[1])
#define ST2 (reply_buffer[2])
#define ST3 (reply_buffer[3])

/*
 * This struct defines the different floppy types. Unlike minix
 * linux doesn't have a "search for right type"-type, as the code
 * for that is convoluted and weird. I've got enough problems with
 * this driver as it is.
 *
 * The 'stretch' tells if the tracks need to be boubled for some
 * types (ie 360kB diskette in 1.2MB drive etc). Others should
 * be self-explanatory.
 * 软盘结构。 定义了不同的软盘类型。
 * 对于1.2MB驱动器的360KB软盘。stretch用于检测磁道是否需要特殊处理。
 * size： 大小，扇区数
 * sect： 每磁道扇区数
 * head： 磁头数
 * track： 磁道数
 * stretch： 对磁道是否需要特殊处理
 * gap： 扇区间隙长度（字节数）
 * rate： 数据传输速率
 * spec1: 参数（高4为步进速率，低四位为磁头卸载时间。
 */
static struct floppy_struct {
	unsigned int size, sect, head, track, stretch;
	unsigned char gap,rate,spec1;
} floppy_type[] = {
	{    0, 0,0, 0,0,0x00,0x00,0x00 },	/* no testing */
	{  720, 9,2,40,0,0x2A,0x02,0xDF },	/* 360kB PC diskettes */
	{ 2400,15,2,80,0,0x1B,0x00,0xDF },	/* 1.2 MB AT-diskettes */
	{  720, 9,2,40,1,0x2A,0x02,0xDF },	/* 360kB in 720kB drive */
	{ 1440, 9,2,80,0,0x2A,0x02,0xDF },	/* 3.5" 720kB diskette */
	{  720, 9,2,40,1,0x23,0x01,0xDF },	/* 360kB in 1.2MB drive */
	{ 1440, 9,2,80,0,0x23,0x01,0xDF },	/* 720kB in 1.2MB drive */
	{ 2880,18,2,80,0,0x1B,0x00,0xCF },	/* 1.44MB diskette */
};
/*
 * Rate is 0 for 500kb/s, 2 for 300kbps, 1 for 250kbps
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),
 * H is head unload time (1=16ms, 2=32ms, etc)
 *
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).
 */

// 软盘中断。 system_call.s中
extern void floppy_interrupt(void);
// 软盘缓冲区。 boot/head.s中
extern char tmp_floppy_area[1024];

/*
 * These are global variables, as that's the easiest way to give
 * information to interrupts. They are the data used for the current
 * request.
 */
static int cur_spec1 = -1;		// 当前参数。高4为步进速率，低四位为磁头卸载时间。
static int cur_rate = -1;		// 当前数据传输速率
static struct floppy_struct * floppy = floppy_type;	// 软盘类型数组
static unsigned char current_drive = 0;		// 当前设备
static unsigned char sector = 0;			// 当前扇区号
static unsigned char head = 0;				// 磁头
static unsigned char track = 0;				// 磁道
static unsigned char seek_track = 0;		// 寻道的 磁道
static unsigned char current_track = 255;	// 当前磁道
static unsigned char command = 0;			// 命令
unsigned char selected = 0;					// 选中标记
struct task_struct * wait_on_floppy_select = NULL;	// 等待队列标记

/**
 * 释放软驱。
 * @param nr 软驱号。
 * 数字输出寄存器DOR的低2位用于指定选择的软驱。
 */
void floppy_deselect(unsigned int nr)
{
	if (nr != (current_DOR & 3))
		printk("floppy_deselect: drive not selected\n\r");
	selected = 0;
	wake_up(&wait_on_floppy_select);
}

/*
 * floppy-change is never called from an interrupt, so we can relax a bit
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point
 * to the desired drive, but it will probably not survive the sleep if
 * several floppies are used at the same time: thus the loop.
 * 修改选中的软驱。 不是从中断中调用的。
 */
int floppy_change(unsigned int nr)
{
repeat:
	floppy_on(nr);	// 选中软驱。会尝试将current_DOR指向所需的驱动器
	// 驱动器无效，不是指定的，就在wait_onfloppy_select上睡眠。
	while ((current_DOR & 3) != nr && selected)
		interruptible_sleep_on(&wait_on_floppy_select);

	// 驱动器无效，不是指定的的nr。循环等待
	if ((current_DOR & 3) != nr)
		goto repeat;
	// 读取FD_DIR 数字输入寄存器。 如果最高位（7）置位，则表示软盘已经更换。
	// 此时关闭电机并返回1
	if (inb(FD_DIR) & 0x80) {
		floppy_off(nr);
		return 1;
	}

	// 关闭软盘并返回0 
	floppy_off(nr);
	return 0;
}

/**
 * 复制内存数据块
 */
#define copy_buffer(from,to) \
__asm__("cld ; rep ; movsl" \
	::"c" (BLOCK_SIZE/4),"S" ((long)(from)),"D" ((long)(to)) \
	:"cx","di","si")

/**
 * 初始化DMA通道
 */
static void setup_DMA(void)
{
	// 当前请求项缓冲区所处的内存地址。
	long addr = (long) CURRENT->buffer;

	cli();	// 关中断

	// 如果缓冲区在1MB以上的地方。则将缓冲区设置为tmp_floppy_area。 
	// 因为8237A芯片智能在1MB地址范围内存之。如果是写盘命令，则还需将数据复制到该临时区域
	if (addr >= 0x100000) {
		addr = (long) tmp_floppy_area;
		if (command == FD_WRITE)
			copy_buffer(CURRENT->buffer,tmp_floppy_area);
	}

	// 屏蔽DMA通道2. 单通道屏蔽寄存器为0x10， 位0-1指定dma的通道。1表示屏蔽，0表示允许
	/* mask DMA 2 */
	immoutb_p(4|2,10);

	// 命令字节。 向dma控制器端口12和11写方式字（读盘0x46， 写盘0x4A）
	/* output command byte. I don't know why, but everyone (minix, */
	/* sanches & canton) output this twice, first to 12 then to 11 */
 	__asm__("outb %%al,$12\n\tjmp 1f\n1:\tjmp 1f\n1:\t"
	"outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1:"::
	"a" ((char) ((command == FD_READ)?DMA_READ:DMA_WRITE)));
	
	// 向DMA通道2，写入当前地址地址寄存器。 端口4
/* 8 low bits of addr */
	immoutb_p(addr,4);
	addr >>= 8;

	// 向DMA通道2，写入当前地址地址寄存器。 端口4
/* bits 8-15 of addr */
	immoutb_p(addr,4);
	addr >>= 8;

	// DMA只能处理1MB的内存空间寻址。 大于的，需要使用0x81（页面寄存器）
/* bits 16-19 of addr */
	immoutb_p(addr,0x81);

	// 计数器低8位。
/* low 8 bits of count-1 (1024-1=0x3ff) */
	immoutb_p(0xff,5);

	// 计数器高8位。 一次传输1024字节，2扇区
/* high 8 bits of count-1 */
	immoutb_p(3,5);

	// 开启dma 2的通道请求。
/* activate DMA 2 */
	immoutb_p(0|2,10);
	sti();	// 开中断。
}

/**
 * 向软盘控制器发送一个字节（控制字节）
 *
 */
static void output_byte(char byte)
{
	int counter;
	unsigned char status;

	// 重置状态
	if (reset)
		return;

	// 尝试1万次
	for(counter = 0 ; counter < 10000 ; counter++) {、
		// 读取状态位。 如果是ready和dir状态。
		status = inb_p(FD_STATUS) & (STATUS_READY | STATUS_DIR);
		if (status == STATUS_READY) {
			// 向输出端口 输出自定的字节
			outb(byte,FD_DATA);
			return;
		}
	}
	reset = 1;
	printk("Unable to send byte to FDC\n\r");
}

/**
 * 读取FDC执行结果
 */
static int result(void)
{
	int i = 0, counter, status;

	// 重置状态
	if (reset)
		return -1;

	// 尝试1万次。 i为尝试次数
	for (counter = 0 ; counter < 10000 ; counter++) {
		// 读取状态字节。 如果是ready。 返回i。
		status = inb_p(FD_STATUS)&(STATUS_DIR|STATUS_READY|STATUS_BUSY);
		if (status == STATUS_READY)
			return i;

		// 如果状态位busy。 
		if (status == (STATUS_DIR|STATUS_READY|STATUS_BUSY)) {
			// 超过最大尝试次数
			if (i >= MAX_REPLIES)
				break;
			// 读取结果值
			reply_buffer[i++] = inb_p(FD_DATA);
		}
	}
	reset = 1;
	printk("Getstatus times out\n\r");
	return -1;
}

/**
 * 软盘操作出错中断调用函数。有软驱中断处理函数调用。
 */
static void bad_flp_intr(void)
{
	CURRENT->errors++;	// 错误次数增加
	if (CURRENT->errors > MAX_ERRORS) {
		floppy_deselect(current_drive);	// 取消对当前设备的选中
		end_request(0);	// 结束当前请求。
	}

	// 如果数据达到4=5次，那么就重置设备。
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
	else
		recalibrate = 1; // 重新校正
}	

/*
 * Ok, this interrupt is called after a DMA read/write has succeeded,
 * so we check the results, and copy any buffers.
 * 读写打断。
 */
static void rw_interrupt(void)
{
	// 通过result函数取到结果值。 或者状态字节0，1，2中存在出错标记
	if (result() != 7 || (ST0 & 0xf8) || (ST1 & 0xbf) || (ST2 & 0x73)) {
		if (ST1 & 0x02) {	// 写保护数据
			printk("Drive %d is write protected\n\r",current_drive);
			floppy_deselect(current_drive);
			end_request(0);
		} else
			bad_flp_intr();	// 错误请求。
		do_fd_request();	// 做下一个请求
		return;
	}

	// 读数据，并且地址大于1MB， 那么就从临时缓冲区复制到真正的地址中
	if (command == FD_READ && (unsigned long)(CURRENT->buffer) >= 0x100000)
		copy_buffer(tmp_floppy_area,CURRENT->buffer);
	// 取消设备选中
	floppy_deselect(current_drive);
	end_request(1);	// 结束请求，并将请求结果标记为1
	do_fd_request();	// 请求下一个
}

/**
 * 设置DMA并输出软盘操作命令和参数
 */
inline void setup_rw_floppy(void)
{
	setup_DMA();			// 设置dma
	do_floppy = rw_interrupt;	// 读写中断函数
	// 设置dma参数
	output_byte(command);
	output_byte(head<<2 | current_drive);
	output_byte(track);
	output_byte(head);
	output_byte(sector);
	output_byte(2);		/* sector size = 512 */
	output_byte(floppy->sect);
	output_byte(floppy->gap);
	output_byte(0xFF);	/* sector size (0xff when n!=0 ?) */
	if (reset)	// 重置标记
		do_fd_request();	// 处理了下一个请求。会先做重置
}

/*
 * This is the routine called after every seek (or recalibrate) interrupt
 * from the floppy controller. Note that the "unexpected interrupt" routine
 * also does a recalibrate, but doesn't come here.
 * 在寻道成功或重新校正中被调用。
 */
static void seek_interrupt(void)
{
	// 检测中断状态
	/* sense drive status */
	output_byte(FD_SENSEI);	// 发送检测中断状态命令。
	// 获取结果。 
	if (result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track) {
		bad_flp_intr();		// 当前请求失败。
		do_fd_request();	// 请求下一个
		return;
	}
	// 更新当前磁道
	current_track = ST1;
	setup_rw_floppy();
}

/*
 * This routine is called when everything should be correctly set up
 * for the transfer (ie floppy motor is on and the correct floppy is
 * selected).
 * 传输信息的所有内容都被设置好后调用。
 * 读写数据传输函数。
 */
static void transfer(void)
{
	// 当前驱动器参数是否为指定驱动器的参数。
	if (cur_spec1 != floppy->spec1) {
		cur_spec1 = floppy->spec1;

		// 发送设置磁盘参数命令
		output_byte(FD_SPECIFY);
		/// 发送参数
		output_byte(cur_spec1);		/* hut etc */
		output_byte(6);			/* Head load time =6ms, DMA */
	}

	// 当前数据传输速率是否与指定驱动器的一致。
	if (cur_rate != floppy->rate)
		outb_p(cur_rate = floppy->rate,FD_DCR);	// 更新当前传输熟虑到DCR中
	
	// 重置标记。
	if (reset) {
		do_fd_request();
		return;
	}

	// 非寻道状态。不需要寻道
	if (!seek) {
		setup_rw_floppy();	// 进行数据读写。dma读写
		return;
	}

	// 需要寻道
	// 设置中断处理函数
	do_floppy = seek_interrupt;
	if (seek_track) {	// 起始磁道号不等于0，发送磁头寻道命令和参数
		output_byte(FD_SEEK);
		output_byte(head<<2 | current_drive);
		output_byte(seek_track);
	} else {
		output_byte(FD_RECALIBRATE);	// 重新校正
		output_byte(head<<2 | current_drive);	// 发送参数，磁头号和当前软驱号
	}

	// 复位标志
	if (reset)
		do_fd_request();
}

/*
 * Special case - used after a unexpected interrupt (or reset)
 * 重新校正中断处理
 */
static void recal_interrupt(void)
{
	// 读取检测中断状态命令
	output_byte(FD_SENSEI);
	// 读取结果。 命令异常结束或者结果字节数不等于2。
	if (result()!=2 || (ST0 & 0xE0) == 0x60)
		reset = 1;	// 重置标记
	else
		recalibrate = 0;	// 重新校正标记
	// 进行下一项请求
	do_fd_request();
}

/**
 * 异常中断
 */
void unexpected_floppy_interrupt(void)
{
	// 读检测中断状态 命令
	output_byte(FD_SENSEI);
	// 读取结果。结果字节数不为2，命令异常结束。
	if (result()!=2 || (ST0 & 0xE0) == 0x60)
		reset = 1;	// 重置标记
	else
		recalibrate = 1;	// 重新校正标记
}

/**
 * 重新校正
 */
static void recalibrate_floppy(void)
{
	recalibrate = 0;	// 校正标记
	current_track = 0;	// 当前磁道
	do_floppy = recal_interrupt;	// 处理中断的函数
	output_byte(FD_RECALIBRATE);	// 重新校正
	output_byte(head<<2 | current_drive); // 参数， 写入磁头和当前设备

	// 重置标记处理
	if (reset)
		do_fd_request();
}

/**
 * 重置中断处理函数
 */
static void reset_interrupt(void)
{
	// 读中断状态函数
	output_byte(FD_SENSEI);
	// 获取结果值
	(void) result();
	// 发送设定软驱参数命令
	output_byte(FD_SPECIFY);
	output_byte(cur_spec1);		/* hut etc */
	output_byte(6);			/* Head load time =6ms, DMA */
	do_fd_request();		// 执行软盘请求
}

/*
 * reset is done by pulling bit 2 of DOR low for a while.
 * 重置软盘
 */
static void reset_floppy(void)
{
	int i;

	reset = 0;		// 清标志位
	cur_spec1 = -1;	// 当前驱动器参数
	cur_rate = -1;	// 当前速率
	recalibrate = 1;	// 重新校正标记
	printk("Reset-floppy called\n\r");
	cli();		// 关中断
	do_floppy = reset_interrupt;	// 中断处理函数
	outb_p(current_DOR & ~0x04,FD_DOR);	// 向数字控制寄存器发送重置命令

	// 执行空指令
	for (i=0 ; i<100 ; i++)
		__asm__("nop");
	outb(current_DOR,FD_DOR);			// 设置状态码
	sti();	// 开中断
}

/**
 * 软盘启动，定时中断处理函数
 */
static void floppy_on_interrupt(void)
{
/* We cannot do a floppy-select, as that might sleep. We just force it */
	selected = 1;		// 选中状态
	// 当前设备，不是数字控制寄存器中的。
	if (current_drive != (current_DOR & 3)) {
		// 设置当前数据控制寄存器的值。
		current_DOR &= 0xFC;
		current_DOR |= current_drive;
		// 写入DOR中
		outb(current_DOR,FD_DOR);
		add_timer(2,&transfer);
	} else
		transfer();	// 执行传输函数
}

/**
 * 处理请求
 */
void do_fd_request(void)
{
	unsigned int block;

	// 寻道标记为0
	seek = 0;

	// 重置标记处理
	if (reset) {
		reset_floppy();
		return;
	}

	// 校正标记处理
	if (recalibrate) {
		recalibrate_floppy();
		return;
	}

	// 请求合法性校验
	INIT_REQUEST;

	// 当前设备
	floppy = (MINOR(CURRENT->dev)>>2) + floppy_type;
	// 当前磁盘不是请求的磁盘，设置寻道标记
	if (current_drive != CURRENT_DEV)
		seek = 1;
	// 当前设备。
	current_drive = CURRENT_DEV;
	// 请求扇区
	block = CURRENT->sector;
	// 如果block + 2 大于软盘总数。 错误的请求。
	if (block+2 > floppy->size) {
		end_request(0);
		goto repeat;
	}
	// 初始化参数设置。 扇区号。 磁头号，磁道号。寻道号。
	sector = block % floppy->sect;
	block /= floppy->sect;
	head = block % floppy->head;
	track = block / floppy->head;
	seek_track = track << floppy->stretch;
	// 寻道号和当前磁道号不一致。 
	if (seek_track != current_track)
		seek = 1;
	// 扇区号+1.
	sector++;
	// 读写命令
	if (CURRENT->cmd == READ)
		command = FD_READ;
	else if (CURRENT->cmd == WRITE)
		command = FD_WRITE;
	else
		panic("do_fd_request: unknown command");

	// 定时器。
	add_timer(ticks_to_floppy_on(current_drive),&floppy_on_interrupt);
}

/**
 * 软盘初始化函数
 */
void floppy_init(void)
{
	// 设置请求函数
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	// 设置陷阱门函数。 0x26 为 软盘中断处理函数
	set_trap_gate(0x26,&floppy_interrupt);
	// 开启中断。 读取中断请求屏蔽码，开启软盘中断请求，并写入0x21中。
	outb(inb_p(0x21)&~0x40,0x21);
}
