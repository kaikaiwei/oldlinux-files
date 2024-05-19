/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 * 内核调度的基本函数，包括sleep_on， weak_up，schedule。
 */
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/fdreg.h>
#include <asm/system.h>		// 系统头文件
#include <asm/io.h>			// io头文件
#include <asm/segment.h>	// 段操作头文件

#include <signal.h>			// 信号头文件

// 取信号nr在信号位图中对应位的二进制数值。信号编号1-32.  就是第几位信号，转换为int值。例：信号5的位图， 1<<4=16=00010000b
#define _S(nr) (1<<((nr)-1))
// 除了SIGKILL和SIGSTOP的信号
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

/**
 * 显示任务号nr的进程号。进程状态和内存堆栈空闲字节数
 * @param nr 任务号
 * @param p  进程描述符
 */
void show_task(int nr,struct task_struct * p)
{
	int i,j = 4096-sizeof(struct task_struct);

	// 打印任务号，进程好，进程状态号
	printk("%d: pid=%d, state=%d, ",nr,p->pid,p->state);
	i=0;
	// 检查指定任务数据结构以后等于0的字节数
	while (i<j && !((char *)(p+1))[i])
		i++;
	// 内核栈中剩余的字节数
	printk("%d (of %d) chars free in kernel stack\n\r",i,j);
}

/**
 * 显示所有任务的任务号，进程号。
 *
 */
void show_stat(void)
{
	int i;

	for (i=0;i<NR_TASKS;i++)
		if (task[i])
			show_task(i,task[i]);
}

// 设置定时芯片8253的计数初值， 10ms一个滴答
#define LATCH (1193180/HZ)

extern void mem_use(void);	// 没有任何地方定义和引用该函数

extern int timer_interrupt(void);	// 时钟中断处理程序。kernel/system_call.s
extern int system_call(void);		// 系统中断处理程序。kernel/system_call.s

/**
 * 定义任务联合体。
 */
union task_union {
	struct task_struct task;	// 因为一个任务数据结构与其堆栈放在同一内存页中。所以从堆栈寄存器ss可以获得其数据顿啊选择符
	char stack[PAGE_SIZE];
};

// 定义初始任务的数据。sched.h中
static union task_union init_task = {INIT_TASK,};

long volatile jiffies=0;	// 系统滴答数， 初始为0
long startup_time=0;		// 系统启动时间。 在main.c中调用kernel_mktime函数进行初始化的
struct task_struct *current = &(init_task.task); 	// 当前任务指针，默认为0号任务
struct task_struct *last_task_used_math = NULL;		// 使用过协处理器的指针

struct task_struct * task[NR_TASKS] = {&(init_task.task), };	// 定义任务指针数组

long user_stack [ PAGE_SIZE>>2 ] ;	// 定义对阵任务0的用户态。 4k

/**
 * 该结构用于设置堆栈ss:esp(数段段选择符，指针)，见head.s中35行。lss _stack_start,%esp
 */
struct {
	long * a;
	short b;
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 *  将当前写出其里内容保存到老协处理器状态数据中。并将当前任务的协处理器内容加载进协处理器
 */
void math_state_restore()
{
	if (last_task_used_math == current) // 如果任务没有变化则返回
		return;
	__asm__("fwait");		// 在发送协处理器命令之前要先发送wait指令
	if (last_task_used_math) {		// 上一个任务使用了协处理器，保存其状态
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	}
	last_task_used_math=current; // 更新last_task_used_math
	if (current->used_math) {		// 如果当前任务使用过协处理器。回复其状态
		__asm__("frstor %0"::"m" (current->tss.i387));
	} else {					// 第一次使用。向协处理器发出初始化命令
		__asm__("fninit"::);
		current->used_math=1;
	}
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 * 调度函数。 任务0是一个闲置任务，只有当没有其他任务可运行时才调用它。
 */
void schedule(void)
{
	int i,next,c;
	struct task_struct ** p;	// 任务结构指针

/* check alarm, wake up any interruptible tasks that have got a signal */
	// 检查alarm（进程的报警定时值）， 唤醒任何已经得到信号的可中断任务
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			// 如果设置过alram，并且alarm时间已经过期（alarm<jiffies), 则在信号位图中置signal alarm，
			// 然后清空alarm标志位
			if ((*p)->alarm && (*p)->alarm < jiffies) {
					(*p)->signal |= (1<<(SIGALRM-1));
					(*p)->alarm = 0;
				}
			// 如果信号位图中除被阻塞的信号外还有其他信号，并且任务在可中断状态
			// 则置任务为可运行状态
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* this is the scheduler proper: */
// 这里是调度函数的主要部分。

	while (1) {
		c = -1;		// 已经找到的最大的运行时间
		next = 0;	// 下一个，缓存第二大运行时间的任务下标
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i) {		// 从后往前找。这样任务0就是最后一个
			// 如果当前任务为空，跳过
			if (!*--p)		
				continue;
			// 任务在运行中，且还有执行时间（counter>-1)
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
		if (c) break; // 如果有counter大于0的任务，那么就退出循环
		// 从任务数组的最后一个到第一个。根据进程优先级设置任务的counter值。counter=counter/2+priority
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}
	switch_to(next);	// 切换到任务号为next的任务，并运行它。 如果没有，则运行任务0
}

/**
 * pause系统调用，转换当前任务的状态为可中断等待状态。并重新调度。
 * 会导致进程进入睡眠状态，直到收到一个信号。该信号用户终止进程或进程调用一个信号捕获函数。
 * 只有当补货一个信号。并且信号捕获处理函数返回，pause才会返回。
 */
int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

/**
 * 睡眠等待。 将任务设置为不可中断状态，并让睡眠队列头指向当前任务。只有明确地唤醒时才会返回。
 * 该函数提供了进程与中断处理程序之间的同步机制。
 * @param p 等待任务的队列头指针。
 */
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	// 如果指针无效，则退出。
	if (!p)
		return;
	// 如果当前任务是0， 则死机（不可能）
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	// 保存已经在等待队列上的任务
	tmp = *p;
	*p = current;	// 睡眠队列头指向当前指针。
	current->state = TASK_UNINTERRUPTIBLE;	// 设置当前指针为不可中断睡眠
	schedule();		// 重新调度。
	// 只有当这个任务被唤醒时，才会返回到这里。
	// 如果还存在等待该资源的进程，就会唤醒所有等待该资源的进程。
	if (tmp)
		tmp->state=0; // TASK_RUNNING 被定义为0
}

/**
 * 将当前任务置为可中断的等待状态，并放入*p指定的等待队列中。
 */
void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	// 如果等待队列为NULL， 则返回。这里表明等待队列无效
	if (!p)
		return;
	// 如果是任务0， 则死机
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");

	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	schedule();
	// 如果等待队列中依旧有等待任务，并且队列头指针所指向的任务不是当前任务。
	// 则该任务置为就绪状态，并重新执行调度程序。
	if (*p && *p != current) {
		(**p).state=0;
		goto repeat;
	}
	*p=NULL;  // 这里应该是*p=tmp， 让队列头指针执行其余等待任务，否则在当前任务之前插入等待队列的任务均被抹掉了。
	if (tmp)
		tmp->state=0;
}

/**
 * 唤醒指定任务
 */
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state=0; // 设置为task_running
		*p=NULL;		// 等待队列头会被设置为null。表示已经唤醒
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 * 这里是软驱的代码了。软驱需要定时操作
 */
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL}; // 等待电动机加速进程的指针数组
static int  mon_timer[4]={0,0,0,0};		// 软驱启动加速需要的时间值（滴答数）
static int moff_timer[4]={0,0,0,0};		// 存放软驱电动机停转之前需维持L时间，默认为10000个滴答
unsigned char current_DOR = 0x0C;		// 数字输出寄存器（初值，允许dma和请求中断，启动fdc）

/**
 * 指定软盘到正常运转状态所需要的延迟（滴答数）。 
 * @param nr 软驱号，0-3，返回值为滴答数
 */
int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;	// 当前选中的软盘号， kernel/blk_drv/floopy.c
	unsigned char mask = 0x10 << nr;	// 驱动号对应输出集群起总启动马达的比特值

	// 最多4个软驱
	if (nr>3)
		panic("floppy_on: nr>3");
	moff_timer[nr]=10000;		/* 100 s = very big :-) */
	cli();				/* use floppy_off to turn it off， 关闭中断*/
	mask |= current_DOR;	// 
	if (!selected) {		// 如果不是当前软驱，需要先复位其他软驱选择位，然后值对应软驱选择位。
		mask &= 0xFC;
		mask |= nr;
	}
	if (mask != current_DOR) {	// 如果数字输出寄存器与当前值不同，则向FDC端口输出新值。
		outb(mask,FD_DOR);
		if ((mask ^ current_DOR) & 0xf0) 
			mon_timer[nr] = HZ/2;
		else if (mon_timer[nr] < 2) // 如果要求启动的电动机还没有启动，则置相应软驱的电动机启动定时器值（hz/2=0.5秒或50个滴答）
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();// 开中断
	return mon_timer[nr];
}

/**
 * 等待指定软驱电动机启动所需时间
 * tips：这里嵌套使用了cli/sti，所以sleep_on在这里不会死机。
 */
void floppy_on(unsigned int nr)
{
	cli();							// 关中断
	while (ticks_to_floppy_on(nr))	// 如果电动机启动定时还没到， 就一直把当前进程置为不可中断睡眠并放入等待电动机运行的队列中
		sleep_on(nr+wait_motor);
	sti();							// 开中断
}

/**
 * 关闭相应的软驱电动机听转 定时器（3秒）
 */
void floppy_off(unsigned int nr)
{
	moff_timer[nr]=3*HZ;
}

/**
 * 软驱时钟处理函数。
 * 更新电动机启动定时器和电动机关闭停转计时器。
 * 该子程序是在时钟定时中断中被调用，因此没一个滴答（10ms）调用一次。更新电动机开启或听转定时器。
 * 如果某一个电动机停转定时到，则将数字输出寄存器电动机启动位复位。
 */
void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i=0 ; i<4 ; i++,mask <<= 1) {
		if (!(mask & current_DOR))		//如果不是dor指定电动机，则跳过
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])
				wake_up(i+wait_motor);	// 如果电动机启动定时到则唤醒进程
		} else if (!moff_timer[i]) {		// 如果电动机停转计时器到
			current_DOR &= ~mask;			// 复位相应电动机启动位，并更新数字输出寄存器
			outb(current_DOR,FD_DOR);
		} else
			moff_timer[i]--;		// 电动机停转及时减少1
	}
}

// 最多可以走64个定时器
#define TIME_REQUESTS 64

// 定时器列表
static struct timer_list {
	long jiffies;	// 滴答数
	void (*fn)();	// 定时处理函数
	struct timer_list * next;	// 下一个定时器
} timer_list[TIME_REQUESTS], * next_timer = NULL;

/**
 * 添加定时器
 * @param jiffies 定时值
 * @param fn 函数指针。
 */
void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)	// 如果处理函数为0， 则直接返回
		return;
	cli();		// 关闭中断
	if (jiffies <= 0)	// 如果jiffies<=0, 则立即执行
		(fn)();
	else {
		// 找到一个空的定时器项
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		// 如果没有对应的定时器，则系统慌张
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		// 列表项按照从小到达的顺序排序。
		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();		// 开总段
}

/**
 * 执行定时器
 * @param cpl， 特权级别
 */
void do_timer(long cpl)
{
	extern int beepcount;			// 扬声器发声时间滴答数。kernel/chr_drv/console.c
	extern void sysbeepstop(void);	// 关闭扬声器 kernel/chr_drv/console.c

	// 减少扬声器发声时间滴答数，如果为0，则关闭扬声器
	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	// cpl 特权级别。根据这个
	if (cpl)
		current->utime++;	// 用户程序运行时间
	else
		current->stime++;	// 内核运行时间

	// 取第一个定时器
	if (next_timer) {
		next_timer->jiffies--;	// 减少滴答数
		// 准备出发
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void); // 这里插入了一个函数指针定义，不过好像没啥用。
			
			fn = next_timer->fn;	// 获取函数指针。
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();		// 执行函数调用
		}
	}

	// 如果当前软盘控制器FDC的数字输出寄存器中电动机启动位有置位的。则执行软盘定时。
	if (current_DOR & 0xf0)
		do_floppy_timer();
	// 减少当前程序计数器计数，如果依旧大于0，则继续执行
	if ((--current->counter)>0) return;
	current->counter=0;	// 当前程序计数器已经结束，需要重新调度
	if (!cpl) return;	// 对于内核程序，不依赖counter值进行调度
	schedule();			// 重新调度
}

/**
 * 系统调用功能。设置报警定时时间值（秒）。
 * 如果设置过alarm值，则返回旧值，否则返回0.
 * @param seconds 秒数
 */
int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

/** 
 * 获取进程pid
 */
int sys_getpid(void)
{
	return current->pid;
}

/**
 * 获取当前进程符进程pid。
 */
int sys_getppid(void)
{
	return current->father;
}

// 获取当前进程uid
int sys_getuid(void)
{
	return current->uid;
}

// 获取当前进程euid，有效用户号
int sys_geteuid(void)
{
	return current->euid;
}

// 获取当前进程组id
int sys_getgid(void)
{
	return current->gid;
}

// 获取当前进程有效进程组id，egid
int sys_getegid(void)
{
	return current->egid;
}

/**
 * 增加nice值
 * 会降低cpu的的使用优先权
 */
int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment; // 降低优先权
	return 0;
}

/**
 * 调度程序的初始化方法
 */
void sched_init(void)
{
	int i;
	struct desc_struct * p;

	// 存放信号有关的状态，必须是16位的。
	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");

	// 设置初始任务（任务0）的任务状态段描述符和局部数据表描述符
	// set_tss_desc和set_ldt_desc在include/system.h中定义
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));	// FIRST_TSS_ENTRY（kernel/sched.h， 默认4）
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));	// FIRST_LDT_ENTRY（kernel/sched.h，默认是5）
	
	// 清任务数组和描述符表项。
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) { // 注意，这里从任务1开始。
		task[i] = NULL;
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}

/* Clear NT, so that we won't have troubles with that later on */
	// 清除NT位。 NT标志是用于控制程序的递归调用（Nested Task）。
	// 当NT置位是，那么当前总段任务执行iret指令时就会引起任务切换。
	// NT指出TSS中的back_link字段是否有效。
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");	// 清除NT标志。
	ltr(0);				// 将任务0的TSS加载到任务寄存器TR。本文169行定义
	lldt(0);			// 将任务0的局部描述符表加载到局部描述符表寄存器。 本文170行
	// 是将GDT中相应LDT描述符的选择符加载到ldtr，只明确加载者一次，以后新任务的LDT加载，是cpu根据TSS中的LDT项自动加载的。
	
	// 初始化8253定时器
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */	// 定时器低字节
	outb(LATCH >> 8 , 0x40);	/* MSB */		// 定时器高字节
	set_intr_gate(0x20,&timer_interrupt);		// 设置时钟中断句柄。 
	outb(inb_p(0x21)&~0x01,0x21);				// 修改中断控制器屏蔽码，允许时钟中断
	set_system_gate(0x80,&system_call); 		// 设置系统调用中断门。
}
