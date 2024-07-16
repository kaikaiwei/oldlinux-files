/*
 * 'tty.h' defines some structures used by tty_io.c and some defines.
 *
 * NOTE! Don't touch this without checking that nothing in rs_io.s or
 * con_io.s breaks. Some constants are hardwired into the system (mainly
 * offsets into 'tty_queue'
 */

#ifndef _TTY_H
#define _TTY_H

#include <termios.h>

// 队列缓冲区长度
#define TTY_BUF_SIZE 1024

/**
 * 字符缓冲队列中。也叫字符标。
 */
struct tty_queue {
	unsigned long data;		// 等待队列缓冲区中当前数据统计值。对于串口终端，存放串口端口地址
	unsigned long head;		// 缓冲区中数据头指针
	unsigned long tail;		// 缓冲区中数据尾指针
	struct task_struct * proc_list;	// 等待本缓冲队列的进程列表
	char buf[TTY_BUF_SIZE];		// 队列的缓冲区。 长度1KB。
};

// 定义了tty等待队列中缓冲区操作宏函数， tail在前， head在后
// a缓冲区指针前移1字节，并循环
#define INC(a) ((a) = ((a)+1) & (TTY_BUF_SIZE-1))
// a缓冲区指针后退1字节，并循环
#define DEC(a) ((a) = ((a)-1) & (TTY_BUF_SIZE-1))
// 清空指定队列的缓冲区
#define EMPTY(a) ((a).head == (a).tail)
// 缓冲区还可存放字符的长度
#define LEFT(a) (((a).tail-(a).head-1)&(TTY_BUF_SIZE-1))
// 缓冲区中最后一个位置
#define LAST(a) ((a).buf[(TTY_BUF_SIZE-1)&((a).head-1)])
// 缓冲区满
#define FULL(a) (!LEFT(a))
// 缓冲区中已存放字符的长度
#define CHARS(a) (((a).head-(a).tail)&(TTY_BUF_SIZE-1))
// 从queue队列项缓冲区中取一字符，从tail处取，并且tail++
#define GETCH(queue,c) \
(void)({c=(queue).buf[(queue).tail];INC((queue).tail);})
// 往queue队列项缓冲区中放置一字符，在head处，并且head+=1
#define PUTCH(c,queue) \
(void)({(queue).buf[(queue).head]=(c);INC((queue).head);})

// 判断中断键盘字符类型
// 中断符
#define INTR_CHAR(tty) ((tty)->termios.c_cc[VINTR])
// 退出符
#define QUIT_CHAR(tty) ((tty)->termios.c_cc[VQUIT])
// 擦除符
#define ERASE_CHAR(tty) ((tty)->termios.c_cc[VERASE])
// 终止符
#define KILL_CHAR(tty) ((tty)->termios.c_cc[VKILL])
// 文件结束符
#define EOF_CHAR(tty) ((tty)->termios.c_cc[VEOF])
// 开始符
#define START_CHAR(tty) ((tty)->termios.c_cc[VSTART])
// 结束符
#define STOP_CHAR(tty) ((tty)->termios.c_cc[VSTOP])
// 挂起符
#define SUSPEND_CHAR(tty) ((tty)->termios.c_cc[VSUSP])

/**
 * 终端设备结构。
 *
 */
struct tty_struct {
	struct termios termios;			// 终端io属性和控制字符数组结构
	int pgrp;						// 所属进程组
	int stopped;					// 停止标志
	void (*write)(struct tty_struct * tty);	// tty 写函数指针
	struct tty_queue read_q;		// tty 读队列。存放从键盘或者串行终端输入的原始字符序列
	struct tty_queue write_q;		// tty 写队列。存放写到控制台显示屏或串行终端程序处理（过滤过的）数据
	struct tty_queue secondary;		// tty辅助队列。存放规范模式字符序列。规范模式队列。 存放read_q中取出经过行规则程序处理过的数据。上层tty_read用于读取secondary队列中的字符。
	};
// tty数据结构数组
extern struct tty_struct tty_table[];

/*	intr=^C		quit=^|		erase=del	kill=^U
	eof=^D		vtime=\0	vmin=\1		sxtc=\0
	start=^Q	stop=^S		susp=^Z		eol=\0
	reprint=^R	discard=^U	werase=^W	lnext=^V
	eol2=\0
*/
#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

// 异步串行通信初始化  kernel/chr_drv/serial.c
void rs_init(void);
// 控制终端初始化  kernel/chr_drv/console.c
void con_init(void);
// tty初始化  kernel/chr_drv/tty_io.c
void tty_init(void);

// tty读取
int tty_read(unsigned c, char * buf, int n);
// tty写入
int tty_write(unsigned c, char * buf, int n);

// 字符设备写入
void rs_write(struct tty_struct * tty);
// 控制终端写入
void con_write(struct tty_struct * tty);

// 拷贝到熟模式
void copy_to_cooked(struct tty_struct * tty);

#endif
