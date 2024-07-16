#ifndef _SCHED_H
#define _SCHED_H

// 任务数量
#define NR_TASKS 64
// 频率
#define HZ 100

// 第一个任务
#define FIRST_TASK task[0]
// 最后一个任务
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc"
#endif

// 进程状态
#define TASK_RUNNING		0		// 运行中
#define TASK_INTERRUPTIBLE	1		// 可中断的睡眠中
#define TASK_UNINTERRUPTIBLE	2	// 不可中断的睡眠中
#define TASK_ZOMBIE		3			// 僵尸进程
#define TASK_STOPPED		4		// 进程结束

// 定义NULL 为空。后面应该修改为了nullptr
#ifndef NULL
#define NULL ((void *) 0)
#endif

// 拷贝页表， 复制进程的页目录表。 
extern int copy_page_tables(unsigned long from, unsigned long to, long size);
// 释放页表， 释放页表所指定的内存块及页表本身
extern int free_page_tables(unsigned long from, unsigned long size);

extern void sched_init(void);		// 调度初始化函数。 kernel/sched.c
extern void schedule(void);			// 调度函数。 kernel/sched.c
extern void trap_init(void);		// 陷阱门初始化函数。 kernel/traps.c
extern void panic(const char * str);	// 系统慌张
extern int tty_write(unsigned minor,char * buf,int count);	// 终端写入

typedef int (*fn_ptr)();	// 一个执行函数

/**
 * 数学协处理器使用的结构，用户保存进程切换时i387的执行状态信息
 */
struct i387_struct {
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
};

/**
 * 任务状态段数据结构
 */
struct tss_struct {
	long	back_link;	/* 16 high bits zero */ // 局部描述符表LDT的选择符
	long	esp0;
	long	ss0;		/* 16 high bits zero */
	long	esp1;
	long	ss1;		/* 16 high bits zero */
	long	esp2;
	long	ss2;		/* 16 high bits zero */
	long	cr3;		// 页目录基址寄存器
	long	eip;
	long	eflags;
	long	eax,ecx,edx,ebx;
	long	esp;
	long	ebp;
	long	esi;
	long	edi;
	long	es;		/* 16 high bits zero */
	long	cs;		/* 16 high bits zero */
	long	ss;		/* 16 high bits zero */
	long	ds;		/* 16 high bits zero */
	long	fs;		/* 16 high bits zero */
	long	gs;		/* 16 high bits zero */
	long	ldt;		/* 16 high bits zero */
	long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
	struct i387_struct i387; // 前一个执行任务的tss选择符
};

// 任务数据结构，进程描述符
struct task_struct {
/* these are hardcoded - don't touch */
	long state;	/* -1 unrunnable, 0 runnable, >0 stopped */ // 任务运行状态，-1表示不可运行
	long counter;		// 任务的运行时间计数（递减）（滴答数）， 运行时间片
	long priority;		// 运行优先数。 任务开始时，counter=priority， 越大，运行时间越长
	long signal;		// 信号。位图，每个比特位代表一种信号。信号值=位偏移值+1
	struct sigaction sigaction[32]; // 信号执行属性结构。对应信号将要执行的操作和标志信息
	long blocked;	/* bitmap of masked signals 进程信号屏蔽码 */
/* various fields */
	int exit_code;			// 任务执行停止的额退出码，父进程会取
	// start_code 代码段地址
	// end_code 代码长度（字节数）
	// end_data 代码长度+数据长度（字节数）
	// brk 总长度
	// start_stack 堆栈段地址
	unsigned long start_code,end_code,end_data,brk,start_stack;
	// pid 进程号
	// father 父进程号
	// pgrp 进程组号
	// session 会话号
	// leader 会话首领
	long pid,father,pgrp,session,leader;
	// uid， 用户id
	// euid  有效用户id
	// suid  保存的用户id
	unsigned short uid,euid,suid;
	// gid 用户组id
	// euid 有效用户组id
	// sgid 保存的用户组id
	unsigned short gid,egid,sgid;
	// 报警定时值
	long alarm;
	// utime 用户态运行时间，滴答数
	// stime 系统台运行时间，滴答数
	// cutime 子进程用户态运行时间
	// cstime 子进程系统态运行时间
	// start_time 进程开始运行时间
	long utime,stime,cutime,cstime,start_time;
	// 是否使用了协处理器
	unsigned short used_math;
/* file system info 文件系统信息 */
	int tty;		/* -1 if no tty, so it must be signed 进程使用tty的子设备号，-1表示没有使用 */
	unsigned short umask;	// 文件创建属性屏蔽码
	struct m_inode * pwd;	// 当前工作目录i节点
	struct m_inode * root;	// 根目录i节点结构
	struct m_inode * executable;	// 执行文件i节点结构
	unsigned long close_on_exec;	// 执行时关闭文件句柄位图标志
	struct file * filp[NR_OPEN];	// 进程使用的文件表结构
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];	// 本任务的局部描述符表。 0空，1代码段cs，2数据和堆栈段ds&ss。
/* tss for this task */
	struct tss_struct tss;	// 本进程的任务状态段信息结构
};

/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x9ffff (=640kB)
 * 用于设置第一个任务的task table。 
 */
#define INIT_TASK \
/* state etc */	{ 0,15,15, \
/* signals */	0,{{},},0, \
/* ec,brk... */	0,0,0,0,0,0, \
/* pid etc.. */	0,-1,0,0,0, \
/* uid etc */	0,0,0,0,0,0, \
/* alarm */	0,0,0,0,0,0, \
/* math */	0, \
/* fs info */	-1,0022,NULL,NULL,NULL,0, \
/* filp */	{NULL,}, \
	{ \
		{0,0}, \
/* ldt */	{0x9f,0xc0fa00}, \
		{0x9f,0xc0f200}, \
	}, \
/*tss*/	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\
	 0,0,0,0,0,0,0,0, \
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
	 _LDT(0),0x80000000, \
		{} \
	}, \
}

// 任务数组
extern struct task_struct *task[NR_TASKS];
// 上一个使用过写出其里的进程
extern struct task_struct *last_task_used_math;
// 当前进程结构指针变量
extern struct task_struct *current;
// 从开机开始算起的滴答数，10ms/滴答
extern long volatile jiffies;
// 开机时间
extern long startup_time;

// 当前时间
#define CURRENT_TIME (startup_time+jiffies/HZ)

// 添加定时器函数， jiffies：定时时间的滴答数。 fn， 函数指针，时间到达后调用的函数
extern void add_timer(long jiffies, void (*fn)(void));
// 不可中断的等待睡眠
extern void sleep_on(struct task_struct ** p);
// 可中断的额等待睡眠
extern void interruptible_sleep_on(struct task_struct ** p);
// 明确唤醒睡眠的进程
extern void wake_up(struct task_struct ** p);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
 // 全局表中第一个局部描述符表LDT描述符的选择符索引号
#define FIRST_TSS_ENTRY 4
// 全局表中第一个局部描述符表LDT描述符
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
// 计算在全局表中第n个任务的TSS描述符的索引号，选择符
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
// 计算在全局表中第n个任务的LDT描述符的索引号
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
// 加载第n个任务的任务寄存器tr
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
// 加载第n个任务的局部描述符表寄存器ldtr
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))
// 取当前运行任务的任务号（时任务数组中的索引值， 与进程pid不同）
// 返回： n 当前任务号。
#define str(n) \
__asm__("str %%ax\n\t" \	// 将任务寄存器中tss段选择符->ax
	"subl %2,%%eax\n\t" \	// eax-first_tss_entry * 8 ->eax 
	"shrl $4,%%eax" \		// (eax/16)->eax 就是当前任务号
	:"=a" (n) \
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))
/*
 *	switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 * 切换当前任务到任务nr，也就是n。 
 * 首先检测n是不是当前任务。
 * 		如果是什么页不错，退出。
 * 如果切换到任务最近（上次）使用过数学协处理器的话，则还需要复位控制寄存器cr0中的ts标志
 * @param %0， 新tss的偏移地址
 * @param %1， 存放新tss的选择符值
 * @param %2， dx，新任务n的选择符
 * @param %3   ecx  新任务指针task[n] 
 */
#define switch_to(n) {\
struct {long a,b;} __tmp; \
__asm__("cmpl %%ecx,_current\n\t" \		// 任务n是当前任务吗？ 
	"je 1f\n\t" \						// 是当前任务，则什么否不做，退出
	"movw %%dx,%1\n\t" \				// 将新新忍辱的选择符，存储到__tmp.b中
	"xchgl %%ecx,_current\n\t" \		// current = task[n], , ecx=被切换处的任务
	"ljmp %0\n\t" \						// 执行长跳转至*&__tmp, 造成任务切换
	"cmpl %%ecx,_last_task_used_math\n\t" \	// 新任务上次使用过协处理器吗？
	"jne 1f\n\t" \						// 如果没有，退出
	"clts\n" \							// 清cr0的ts标志
	"1:" \								// 标记1，用来退出
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \
	"d" (_TSS(n)),"c" ((long) task[n])); \
}

// 页面地址对准，未使用
#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

// 设置位于地址addr处描述符中的各基址字段（基地址是base）
// %0， 地址addr偏移2
// %1， 地址addr偏移4
// %2， 地址addr偏移7
// %3， edx 基地址base
#define _set_base(addr,base) \
__asm__("movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %%dl,%1\n\t" \
	"movb %%dh,%2" \
	::"m" (*((addr)+2)), \
	  "m" (*((addr)+4)), \
	  "m" (*((addr)+7)), \
	  "d" (base) \
	:"dx")

// 设置位于地址addr处描述符中的段限长字段 段长是limit
// %0: 地址
// %1：地址addr偏移6处
// %2: edx 段长值limit
#define _set_limit(addr,limit) \
__asm__("movw %%dx,%0\n\t" \	// 段长limit低16位到addr
	"rorl $16,%%edx\n\t" \		// edx中段长高4位->dl
	"movb %1,%%dh\n\t" \		// 取原[addr +6] 字节->dh, 其中高4位是写标志
	"andb $0xf0,%%dh\n\t" \		// 清dh的低4字节
	"orb %%dh,%%dl\n\t" \		// 将原高4位标志和段长的高4位合成
	"movb %%dl,%1" \			// 1字节， 并放回[addr +6]处
	::"m" (*(addr)), \
	  "m" (*((addr)+6)), \
	  "d" (limit) \
	:"dx")

// 设置局部描述符表中ldt的基址字段
#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , base )
// 设置局部描述符表中ldt描述符的段长字段
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) , (limit-1)>>12 )

// 从地址addr处描述符中取段机制。 功能与set_base正好相反
#define _get_base(addr) ({\
unsigned long __base; \
__asm__("movb %3,%%dh\n\t" \
	"movb %2,%%dl\n\t" \
	"shll $16,%%edx\n\t" \
	"movw %1,%%dx" \
	:"=d" (__base) \
	:"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7))); \
__base;})

// 取局部描述符表中
#define get_base(ldt) _get_base( ((char *)&(ldt)) )

// 取段选择符segment的段长值
// %0 存放段长值
// %1 段选择符 segment
#define get_limit(segment) ({ \
unsigned long __limit; \
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
__limit;})

#endif
