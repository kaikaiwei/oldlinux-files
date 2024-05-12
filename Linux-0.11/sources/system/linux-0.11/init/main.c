/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__			// 定义该变量为为例包括定义在unistd.h中的内嵌汇编代码信息
#include <unistd.h>			// 标准类型与常量文件。如果定义了__LIBRARY__则包含系统调用号和内嵌汇编
#include <time.h>			// 时间类型的头文件，主要定义了tm的结构和一些时间有关的函数。

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 * 内嵌语句。从内核空间创建进程将导致没有写时复制（cow），直接执行到execve。
 * 者对堆栈来说是个大问题。处理方式是在fork调用之后不让main使用堆栈，因此就不能有函数调用。
 * 这就意味着fork要使用内嵌代码，否则在fork退出时就会使用堆栈。
 * 只有fork和pause需要使用内嵌形式。
 */
static inline _syscall0(int,fork)		// unistd中的内嵌宏。以汇编形式调用linux的系统中断0x80. 该中断是所有中断的入口。
static inline _syscall0(int,pause)		// pause 系统调用
static inline _syscall1(int,setup,void *,BIOS)	// int setup(void *BIOS) 系统调用
static inline _syscall0(int,sync)				// int sync() 系统调用

#include <linux/tty.h>		// tty头文件，定义了tty_io，串行通信方面的参数
#include <linux/sched.h>	// 定义了任务结构体task_struct，第一个初始任务的数据
#include <linux/head.h>		// head头文件，定义段描述符的简单结构和几个选择符常量
#include <asm/system.h>		// 系统头文件，以宏的形式定义了许多有关设置或修改描述符/中断门等的嵌入式汇编字程序
#include <asm/io.h>			// io头文件。以宏的嵌入汇编程序形式定义对io端口操作的函数

#include <stddef.h>			// 标准定义头文件，定义了NULL, ooosetof(TYPE, MEMBER)等
#include <stdarg.h>			// 标准参数头文件。以宏的形式定义变量参数列表。
#include <unistd.h>			// 重复引用？
#include <fcntl.h>			// 文件控制头文件
#include <sys/types.h>		// 类型头文件。定义了基本的系统数据类型

#include <linux/fs.h>		// 文件系统头文件

static char printbuf[1024];	// 静态字符串数组。用作内核显示信息的缓存

extern int vsprintf();		// 送格式化输出到一字符串中  kernel/vsprintf.c中
extern void init(void);		// 原型载本文件中
extern void blk_dev_init(void);		// 块设备初始化 blk_dev/llrw_blk.c中
extern void chr_dev_init(void);		// 字符设备初始化 chr_dev/tty_io.c中
extern void hd_init(void);			// 硬盘初始化  chr_dev/hd.c中
extern void floppy_init(void);		// 软驱初始化程序 blk_dev/floppy.c中
extern void mem_init(long start, long end);			// 内存初始化程序， mm/memory.c中
extern long rd_init(long mem_start, int length);	// 虚拟盘初始化程序  blk_dev/ramdisk.c中
extern long kernel_mktime(struct tm * tm);			// 建立内核时间（秒）
extern long startup_time;							// 内核启动时间（开机时间）（秒）

/*
 * This is set up by the setup-routine at boot-time
 * 这些有setup.s载引导程序中设置的。
 */
#define EXT_MEM_K (*(unsigned short *)0x90002)		// 1MB以后的扩展内存大小（KB）
#define DRIVE_INFO (*(struct drive_info *)0x90080)	// 硬盘参数基地址
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)	// 根文件系统所在设备号

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */
// 读取cmos实时时钟信息
// 0x70是写端口号。 0x80｜addr是要读取的cmos内存地址
// 0x71是读端口号
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

// 将BCD码转成数字， val=val&5 + （val/16）*10
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

/* 程序读取cmos时钟，并设置开机时间*/
static void time_init(void)
{
	struct tm time;

	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;		// 月份是0-11
	startup_time = kernel_mktime(&time); // 建立内核时间。在init/kernel/mktime.c中
}

static long memory_end = 0;			// pc具有的内存，结束地址
static long buffer_memory_end = 0;	// 高数缓冲区末端地址
static long main_memory_start = 0;	// 主内存（用于分页）开始的位置

// 用于存放硬盘参数表信息
struct drive_info { char dummy[32]; } drive_info;

void main(void)		/* This really IS void, no error here. */
{			/* The startup routine assumes (well, ...) this */
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 * 这里仍然禁止中断。
 */
 	ROOT_DEV = ORIG_ROOT_DEV;	// 保存根设备号，setup.s中定义
 	drive_info = DRIVE_INFO;	// 硬盘参数基地址
	memory_end = (1<<20) + (EXT_MEM_K<<10);	// 内存结束地址，或者说内存大小。1MB+扩展内存（KB） * 1024
	memory_end &= 0xfffff000;			// 忽略不到4KB（1页）的内存数。
	// 只支持16MB的内存地址
	if (memory_end > 16*1024*1024)
		memory_end = 16*1024*1024;
	//  如果大于12MB，则设置缓冲区末端=4MB。
	if (memory_end > 12*1024*1024) 
		buffer_memory_end = 4*1024*1024;
	else if (memory_end > 6*1024*1024)	// //  如果大于6MB，则设置缓冲区末端=2MB。
		buffer_memory_end = 2*1024*1024;
	else
		buffer_memory_end = 1*1024*1024; //  内存大小小于6MB，设置缓冲区末端=1MB。
	// 主内存起始位置=缓冲区末端地址
	main_memory_start = buffer_memory_end;

// 如果定义了虚拟盘，则初始化虚拟盘。这个时候主内存会减少。
#ifdef RAMDISK
	// rd_init 定义在kernel/blk_drv.c中，参数是起始地址和长度，返回是ram占用的大小（其实就是长度信息）
	// rd_init 会初始化ram的起始地址，长度，操作函数，并将内存信息填充为'\0'
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024); 
#endif
	mem_init(main_memory_start,memory_end);	// 内存初始化，mm/memory.c
	trap_init();						// 陷阱门（中断向量）初始化。kernel/traps.c
	blk_dev_init();						// 块设备初始化。 kernel/blk_drv/ll_rw_blk.c
	chr_dev_init();						// 字符设备初始化。 kernel/chr_drv/tty_io.c
	tty_init();							// tty初始化。 kernel/chr_dev/tty_io.c
	time_init();						// 设置开机启动时间。 本文85行。
	sched_init();						// 调度程度初始化，（加载了任务0的tr，ldtr）。kernel/sched.c
	buffer_init(buffer_memory_end);		// 缓冲管理初始化，建立内核链表等。fs/buffer.c
	hd_init();							// 硬盘初始化。kernel/blk_dev/hd.c
	floppy_init();						// 软盘初始化。kernel/blk_dev/floppy.c
	sti();								// 初始化工作结束，开启中断
	move_to_user_mode();				// 移动到用户模式下。 include/asm/system.h中
	if (!fork()) {		/* we count on this going ok。 fork出init，为0表示人物号 */
		init();			// 在新建的子程序（任务1）中执行。
	}
	// 下面的代码开始以任务0的身份运行。
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 * 对于任何其他任务。pause意味着我们必须等待接收到一个信号才会返回就绪运行态。
 * 但是任务0是唯一的例外情况。因为任务0在任何空闲时间都会被激活（载没有其他任务运行时）。
 * 因此对于任务0，仅意味着我们返回来查看是否有其他任务可以运行。如果没有的话就回到这里，一直循环执行pause就好。
 */
	for(;;) pause();
}

/** 
 * 格式化输出函数。
 * 是vsprint的一个例子。
 */
static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	// 写函数。1是标准输出。 buf缓冲区为printfbuf， 最后一个参数是字节数。
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i; // 写入的字节数
}

static char * argv_rc[] = { "/bin/sh", NULL }; // 调用执行程序时参数的字符串数组
static char * envp_rc[] = { "HOME=/", NULL };	// 调用执行程序时的环境字符串数组

static char * argv[] = { "-/bin/sh",NULL };			// 调用执行程序时参数的字符串数组
static char * envp[] = { "HOME=/usr/root", NULL };	// 调用执行程序时的环境字符串数组

/** init函数运行在任务0创建的子进程中。*/
void init(void)
{
	int pid,i;

	// 读取硬盘参数，包括分区表信息并建立虚拟盘和安装根文件系统设备。
	// 该函数在本代码第29行对应的。 对应的函数是sys_setup(), 在kernel/blk_drv/hd.c中
	setup((void *) &drive_info);
	(void) open("/dev/tty0",O_RDWR,0); // 使用读写方式访问打开/dev/tty0设备。对应终端控制台，返回的句柄0号，也就是stdin标准输入设备
	(void) dup(0);		// 复制句柄，产生1号句柄，表示stdout标准输出设备
	(void) dup(0);		// 复制句柄，产生2号句柄，表示stderr表转错误输出设备
	// 打印缓冲区块数 和缓冲区总字节数。每块1024字节。
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	// 打印空闲内存字节数
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);
	// 创建子进程。执行/bin/sh程序
	if (!(pid=fork())) {  // 创建子进程
		close(0);			// 子进程关闭标准输入
		if (open("/etc/rc",O_RDONLY,0))	// 只读方式打开/etc/rc文件。
			_exit(1);					// 如果打开文件失败，则推出。 /lib/_exit.c 
		execve("/bin/sh",argv_rc,envp_rc); // 装载/bin/bash程序并执行
		_exit(2);		// 如果execve执行失败，则
	}
	// 父进程执行的语句。等待子进程的结束。这里是等待所有进程，包括僵尸进程。
	if (pid>0)
		while (pid != wait(&i)) // 如果子进程是pid，那么表示子进程已经推出了。
			/* nothing */;
	// 如果执行到这里，说明子进程的执行已经停止或终止了。
	// 这里首先会再创建一个进程。并一直运行，是一个大的死循环。
	while (1) {
		// 创建一个子进程
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) { // 子进程中。
			// 关闭标准输入，标准输出，标准错误流。
			close(0);close(1);close(2);
			setsid();	// 创建新的绘画，并设置进程组号。
			// 重新打开标准输入，标准输出，标准错误流。
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			// 执行/bin/sh文件。
			_exit(execve("/bin/sh",argv,envp));
		}
		// 等待子进程的结束。
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
	_exit(0);	/* NOTE! _exit, not exit() 真实情况是，并不会执行到这里。 */
}
