/*
 *  linux/kernel/tty_io.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'tty_io.c' gives an orthogonal feeling to tty's, be they consoles
 * or rs-channels. It also implements echoing, cooked mode etc.
 *
 * Kill-line thanks to John T Kohl.
 *  字符设备的上层接口，主要含有中断读写函数tty_read和tty_write。 
 *  读操作行规范函数copy_to_cooked也在这里
 */
#include <ctype.h>		// 字符类型头文件
#include <errno.h>		// 错误号头文件
#include <signal.h>		// 信号头文件

// 信号在信号位图中的对应比特位。
// 警告信号屏蔽位。 均在signal中。
#define ALRMMASK (1<<(SIGALRM-1))
// 终止信号屏蔽位
#define KILLMASK (1<<(SIGKILL-1))
// 键盘中断信号屏蔽位
#define INTMASK (1<<(SIGINT-1))
// 键盘退出信号品比位
#define QUITMASK (1<<(SIGQUIT-1))
// tty发出的停止进程（tty stop）信号品比位
#define TSTPMASK (1<<(SIGTSTP-1))

#include <linux/sched.h>	// 进程调度头文件
#include <linux/tty.h>		// tty头文件，tty io， 串行通信方面的额参数
#include <asm/segment.h>	// 段操作头文件键。 段寄存器操作的嵌入式汇编函数
#include <asm/system.h>		// 系统头文件，定义设置或修改描述符/中断门等的嵌入式汇编宏

// 取termios结构中的本地模式标志
#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)
// 取termios结构中的输入模式标志
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)
// 取termios结构中的输出模式标志
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)

// termios结构中，本地模式标志类型的宏处理
// 本地模式标志中集中规范（熟）模式标志位
#define L_CANON(tty)	_L_FLAG((tty),ICANON)
// 本地模式标志中信号标志位
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)
// 本地模式中的回显标志位
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)
// 本地模式中，规范模式时，取回西安查处标志位
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)
// 本地模式中，规范模式时，取kill查处当前行标志位
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)
// 本地模式中，规范模式时，取回显控制字符标志位
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)
// 本地模式中，规范模式时，取kill擦除行并回显标志位
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)

// termios结构中，输入模式标志类型的宏处理
// 输入模式中，大写到小写转换标志位
#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)
// 输入模式中，取换行符NL转回车符CR标志位
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)
// 输入模式中，取回车符CR转换行符NL标志位
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)
// 输入模式中，取忽略回车符CR标志位
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)

// termios结构中，输出模式标志类型的宏处理
// 输出模式中，标志集中执行输出处理标志
#define O_POST(tty)	_O_FLAG((tty),OPOST)
// 输出模式中，取换行符NL转回车符CR-NL标志
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)
// 输出模式中，取回车符CR转换行符NL标志
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)
// 输出模式中，取换行符NL执行回车功能的标志
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)
// 输出模式中，取小写转大写字符标志
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)

/**
 * tty数据机构的tty_table数组。包含三个初始化变量。
 * 分别对应控制台，串口终端1和串口终端2的初始化数据
 */
struct tty_struct tty_table[] = {
	{
		{ICRNL,		/* change incoming CR to NL */		// 将输入的CR转换为NL
		OPOST|ONLCR,	/* change outgoing NL to CRNL */// 将输出的NL转换为CR-NL
		0,												// 控制模式标志初始化为0
		ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,		// 本地模式标志
		0,		/* console termio */					// 控制台termios
		INIT_C_CC},										// 控制字符数组
		0,			/* initial pgrp */					// 初始进程组
		0,			/* initial stopped */				// 初始停止标志
		con_write,										// 写指针
		{0,0,0,0,""},		/* console read-queue */	// tty 控制台读队列
		{0,0,0,0,""},		/* console write-queue */	// tty 控制台写队列
		{0,0,0,0,""}		/* console secondary queue */ // tty 控制台辅助队列
	},{
		{0, /* no translation */			// 输入模式标志，无需转换
		0,  /* no translation */			// 输出模式标志，无需转换
		B2400 | CS8,						// 控制模式标志。 比特率2400bps， 8位数据位
		0,									// 本地模式标志
		0,									// 控制台termios
		INIT_C_CC},							// 控制字符数组
		0,									// 初始进程组
		0,									// 初始停止标志
		rs_write,							// 写指针
		{0x3f8,0,0,0,""},		/* rs 1 */	// 串口终端1，读缓冲队列
		{0x3f8,0,0,0,""},					// 串口终端1，写缓冲队列
		{0,0,0,0,""}						// 串口终端1，辅助队列
	},{	
		{0, /* no translation */			// 出入模式标志，无需转换
		0,  /* no translation */			// 输出模式标志，无需转换
		B2400 | CS8,						// 控制模式标志。比特率2400bps，8位数据位
		0,									// 本地控制模式
		0,									// 控制台termios
		INIT_C_CC},							// 初始字符数组
		0,									// 初始进程组
		0,									// 初始停止标志
		rs_write,							// 写函数指针
		{0x2f8,0,0,0,""},		/* rs 2 */	// 串口终端2，读缓冲队列	
		{0x2f8,0,0,0,""},					// 串口终端2，写缓冲队列
		{0,0,0,0,""}						// 串口终端2， 辅助队列
	}
};

/*
 * these are the tables used by the machine code handlers.
 * you can implement pseudo-tty's or something by changing
 * them. Currently not done.
 * 汇编程序使用的缓冲队列地址表。通过修改可以实现伪终端或者其他终端类型。
 * tty缓冲队列地址表。 rs_io.s汇编程序使用，用于取得读写缓冲队列地址
 */
struct tty_queue * table_list[]={
	&tty_table[0].read_q, &tty_table[0].write_q,	// 控制台终端读，写缓冲队列地址，
	&tty_table[1].read_q, &tty_table[1].write_q,	// 串口1终端读，写缓冲队列地址
	&tty_table[2].read_q, &tty_table[2].write_q		// 串口2终端读，写缓冲队列地址
	};

/**
 * 终端初始化
 * 分别调用了rs_init和con_init
 */
void tty_init(void)
{
	rs_init();	// kernel/serial.c 初始化中断程序和串行接口1和2
	con_init(); // kernel/console.c 初始化控制台终端
}

/**
 * tty 键盘中断字符处理函数。 
 * @param tty 中断结构指针
 * @param mask 信号屏蔽位
 */
void tty_intr(struct tty_struct * tty, int mask)
{
	int i;

	// 如果tty所属组号小于或等于0，则退出
	if (tty->pgrp <= 0)
		return;
	// 扫描任务数组，向tty相应组的所有任务发送指定的信号
	// 如果该任务指针不为空，且组号等于tty组号，则设置改任务指定的信号mask
	for (i=0;i<NR_TASKS;i++)
		if (task[i] && task[i]->pgrp==tty->pgrp)
			task[i]->signal |= mask;
}

/**
 * 如果队列缓冲区空则让进程进入可中断的睡眠状态
 */
static void sleep_if_empty(struct tty_queue * queue)
{
	cli();	// 关中断
	// 如果当前进程没有要处理的信号，且指定队列的缓冲区空， 则让进程进入可中断睡眠状态，并让队列的进程等待指向改指针。
	while (!current->signal && EMPTY(*queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();	// 开中断
}

/**
 * 如果队列缓冲区满了则让进程进入可中断的睡眠状态
 */
static void sleep_if_full(struct tty_queue * queue)
{
	// 缓冲区不满
	if (!FULL(*queue))
		return;
	cli();	// 关中断
	// 没有要处理的中断信号，并且指定的队列缓冲区空西安长度小于128， 则进入可中断的睡眠
	while (!current->signal && LEFT(*queue)<128)
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

/**
 * 等待按键，如果控制台的读队列缓冲区空则让进程进入可中断的睡眠状态
 */
void wait_for_keypress(void)
{
	sleep_if_empty(&tty_table[0].secondary);
}

/**
 * 复制成规范模式字符序列。
 * @param tty
 */
void copy_to_cooked(struct tty_struct * tty)
{
	signed char c;	

	// 读队列不为空，切辅助对话缓冲区为空，则循环处理
	while (!EMPTY(tty->read_q) && !FULL(tty->secondary)) {
		// 从读队列尾部获取已字符到c，并前移尾指针
		GETCH(tty->read_q,c);

		// 对输入字符，利用输入模式标志集进行处理
		if (c==13)		// 字符是回车符
			if (I_CRNL(tty))	// 输入模式回车符转换行符。字符转位NL。
				c=10;
			else if (I_NOCR(tty))	// 忽略回车标志。
				continue;
			else ;
		else if (c==10 && I_NLCR(tty))	// 换行符，并且换行转回车
			c=13;
		
		// 大写转小写标志
		if (I_UCLC(tty))
			c=tolower(c);
		// 本地模式标志集中规范模式（熟模式开启）
		if (L_CANON(tty)) {
			// 如果改字符时键盘终止控制符KILL（^U), 则进行删除输入行处理
			if (c==KILL_CHAR(tty)) {
				// 非（辅助队列为空，上一个字符为换行符。 或者文件结束标志。
				/* deal with killing the input line */
				while(!(EMPTY(tty->secondary) ||
				        (c=LAST(tty->secondary))==10 ||
				        c==EOF_CHAR(tty))) {
					// 本地回显模式打开
					if (L_ECHO(tty)) {
						// 如果字符是控制字符
						if (c<32)
							PUTCH(127,tty->write_q); // 向tty写队列中放入擦除字符ERASE。 
						// 向tty写队列中放入擦除字符ERASE。
						PUTCH(127,tty->write_q);
						// 调用tty写函数
						tty->write(tty);
					}
					// 将tty辅助队列头指针后退1字节
					DEC(tty->secondary.head);
				}
				continue;
			}
			// 如果该字符时擦除字符
			if (c==ERASE_CHAR(tty)) {
				// 辅助队列为宏或者辅助队列中上一个是换行符，或者tty文件结尾。则忽略
				if (EMPTY(tty->secondary) ||
				   (c=LAST(tty->secondary))==10 ||
				   c==EOF_CHAR(tty))
					continue;
				// 本地回显模式打开
				if (L_ECHO(tty)) {
					// 如果字符是控制字符，向tty写队列中放入擦除字符
					if (c<32)
						PUTCH(127,tty->write_q);
					// 向tty写队列中放入擦除字符ERASE。 
					PUTCH(127,tty->write_q);
					// 调用tty写函数
					tty->write(tty);
				}
				// 将tty辅助队列头指针后退1字节
				DEC(tty->secondary.head);
				continue;
			}
			// 停止字符，标记停止位
			if (c==STOP_CHAR(tty)) {
				tty->stopped=1;
				continue;
			}
			// 开始字符，停止位置0
			if (c==START_CHAR(tty)) {
				tty->stopped=0;
				continue;
			}
		}

		// 本地模式，接收信号
		if (L_ISIG(tty)) {
			// 如果是键盘中断符 ctrl + c
			if (c==INTR_CHAR(tty)) {
				// 项当前进程发送键盘中断信号，并继续处理
				tty_intr(tty,INTMASK);
				continue;
			}
			// 如果是退出符^\, 项当前进程发送键盘退出信号，并处理下一字符
			if (c==QUIT_CHAR(tty)) {
				tty_intr(tty,QUITMASK);
				continue;
			}
		}
		// 如果是换行符NL，或者文件结束符^D， 辅助缓冲队列行号加1
		if (c==10 || c==EOF_CHAR(tty))
			tty->secondary.data++; // 行号

		// 本地模式，回显标志。
		if (L_ECHO(tty)) {
			// 如果是NL，将NL和CR放入到写缓冲队列中去
			if (c==10) {
				PUTCH(10,tty->write_q);
				PUTCH(13,tty->write_q);
			} else if (c<32) { // 如果是控制字符
				// 回显控制字符标志
				if (L_ECHOCTL(tty)) { // 字符^和字符+64放入写缓冲队列
					PUTCH('^',tty->write_q);
					PUTCH(c+64,tty->write_q);
				}
			} else
				PUTCH(c,tty->write_q); // 将该字符放入tty写缓冲队列中
			tty->write(tty); // 写操作函数
		}
		PUTCH(c,tty->secondary); // 字符放入辅助队列中
	}
	wake_up(&tty->secondary.proc_list); /// 唤醒等待该辅助缓冲队列的进程
}

/**
 * tty 读函数
 * @param channel 子设备号
 * @param buf 缓冲区指针
 * @param nr 欲读字节数
 */
int tty_read(unsigned channel, char * buf, int nr)
{
	struct tty_struct * tty;
	char c, * b=buf;
	int minimum,time,flag=0;
	long oldalarm;

	// 检查串口号。
	if (channel>2 || nr<0) return -1;
	// 得到对应的终端数据结构。
	tty = &tty_table[channel];
	// 保存之前的定时值。 
	oldalarm = current->alarm; 
	time = 10L*tty->termios.c_cc[VTIME]; // VTIME超时定时值
	minimum = tty->termios.c_cc[VMIN];	 // VMIN，为了满足操作，最小需要的字符数

	// 如果设置了读超时定时值time，但是没有设置最好读取个数minimum，那么载读到至少一个字符或者定时超时候读操作将立即返回。
	// 所以这里设置minimum=1
	if (time && !minimum) {
		minimum=1;
		// 如果进程原定时值是0或者time+当前系统时间值小于元定时值的话。
		// 重置进程定时值为time+ 当前系统时间，并置flag标志
		if (flag=(!oldalarm || time+jiffies<oldalarm))
			current->alarm = time+jiffies;
	}
	if (minimum>nr)
		minimum=nr;

	// 读取字符
	while (nr>0) {
		// 如果flag不为0（原进程定值时说0或者time+当前系统时间值小于进程原定时值）
		// 并且进程有定时信号ALRMMASK， 则复位进程的定时信号并终端循环
		if (flag && (current->signal & ALRMMASK)) {
			current->signal &= ~ALRMMASK;
			break;
		}
		// 当前进程有信号要处理，退出并返回
		if (current->signal)
			break;
		// 如果辅助缓冲队列为空或者设置了规范模式标志。 
		// 辅助队列中字符数为0，或者辅助队列空闲空间>20， 进入可中断睡眠
		if (EMPTY(tty->secondary) || (L_CANON(tty) &&
		!tty->secondary.data && LEFT(tty->secondary)>20)) {
			sleep_if_empty(&tty->secondary);
			continue;
		}

		do {
			GETCH(tty->secondary,c);  // 从缓冲队列中取得字符c
			if (c==EOF_CHAR(tty) || c==10)	// 如果是文件结束标志或者换行。
				tty->secondary.data--;		// 行数减1
			if (c==EOF_CHAR(tty) && L_CANON(tty)) // 如果是文件结束并且规范模式置位。返回已读字符数，并退出
				return (b-buf);
			else {
				put_fs_byte(c,b++); // 将字符c放入到缓冲区中
				if (!--nr)			// 如果欲读字符数为0，则中断循环
					break;
			}
		} while (nr>0 && !EMPTY(tty->secondary)); // nr 为0或者辅助缓冲队列为空

		// 原进程定时值time不为0，并且非规范模式。重新设置进程定时值。
		if (time && !L_CANON(tty))
			if (flag=(!oldalarm || time+jiffies<oldalarm))
				current->alarm = time+jiffies;
			else
				current->alarm = oldalarm; // 定时值为原定时值
		// 规范模式标志置位。只要读取到一个字符，就中断循环
		if (L_CANON(tty)) {
			if (b-buf)
				break;
		} else if (b-buf >= minimum)	// 满足最小读取个数要求。
			break;
	}
	current->alarm = oldalarm;	// 定时值为原定时值

	// 如果进程有信号，并且没有读取任何字符，则返回出错号
	if (current->signal && !(b-buf))
		return -EINTR;
	return (b-buf);	// 返回已读字符数
}

/**
 * tty写函数
 * @param channel 子设备号
 * @param buf 缓冲区指针
 * @param nr 写字节数
 */
int tty_write(unsigned channel, char * buf, int nr)
{
	static cr_flag=0;
	struct tty_struct * tty;
	char c, *b=buf;

	// 设备号校验
	if (channel>2 || nr<0) return -1;
	tty = channel + tty_table;

	// 欲写字节数大于0
	while (nr>0) {
		// 如果队列满，则进入可中断的睡眠
		sleep_if_full(&tty->write_q);
		// 如果当前进程有需要处理的信号量，退出
		if (current->signal)
			break;
		// 如果欲读字节数大于0，且写队列不满
		while (nr>0 && !FULL(tty->write_q)) {
			// 从缓冲区取一个字符
			c=get_fs_byte(b);
			// 
			if (O_POST(tty)) {
				if (c=='\r' && O_CRNL(tty)) // 回车符，且回车转换行
					c='\n';
				else if (c=='\n' && O_NLRET(tty)) // 换行符，切换行转回车
					c='\r';
				if (c=='\n' && !cr_flag && O_NLCR(tty)) { // 换行符，回车标志没有置位，换行转回车
					cr_flag = 1;
					PUTCH(13,tty->write_q);
					continue;
				}
				// 小写转大写置位
				if (O_LCUC(tty))
					c=toupper(c);
			}
			// 移动指针位置
			b++; nr--;
			cr_flag = 0;
			// 将字符放入到写缓冲队列
			PUTCH(c,tty->write_q);
		}
		// 调用tty的写函数
		tty->write(tty);
		// 如果依旧有需要写的字符，重新调度
		if (nr>0)
			schedule();
	}
	// 返回写了多少字符
	return (b-buf);
}

/*
 * Jeh, sometimes I really like the 386.
 * This routine is called from an interrupt,
 * and there should be absolutely no problem
 * with sleeping even in an interrupt (I hope).
 * Of course, if somebody proves me wrong, I'll
 * hate intel for all time :-). We'll have to
 * be careful and see to reinstating the interrupt
 * chips before calling this, though.
 *
 * I don't think we sleep here under normal circumstances
 * anyway, which is good, as the task sleeping might be
 * totally innocent.
 * tty 中断处理函数。 需要指定tty的子设备号
 */
void do_tty_interrupt(int tty)
{
	copy_to_cooked(tty_table+tty);
}

/**
 * 浪费表情，竟然是空的
 */
void chr_dev_init(void)
{
}
