/*
 *  linux/kernel/serial.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	serial.c
 *
 * This module implements the rs232 io functions
 *	void rs_write(struct tty_struct * queue);
 *	void rs_init(void);
 * and all interrupts pertaining to serial IO.
 *  用于对串行端口进行初始化操作
 * 设置默认的串口通信参数
 * 设置串口通信中断门响亮
 */

#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

// 1024/4 = 256
// 当写队列中含有WAKEUP_CHARS个字符的时候，就开始发送
#define WAKEUP_CHARS (TTY_BUF_SIZE/4)

extern void rs1_interrupt(void);	// 串行口1 中断处理函数
extern void rs2_interrupt(void);	// 串行口2 中断处理函数

/**
 * 串口初始化
 * @param 端口号  串口1: 0x3F8， 串口2: 0x2F8
 */
static void init(int port)
{
	outb_p(0x80,port+3);	/* set DLAB of line control reg */	// 设置线路控制寄存器 dlab, 位7
	outb_p(0x30,port);	/* LS of divisor (48 -> 2400 bps */		// 设置波特率因子低字节。0x30（48）对应2400的波特率
	outb_p(0x00,port+1);	/* MS of divisor */		// 设置波特率高字节 0x00 
	outb_p(0x03,port+3);	/* reset DLAB */		// 设置线路控制寄存器 dlab， 数据位为8
	outb_p(0x0b,port+4);	/* set DTR,RTS, OUT_2 */	// 设置DTR，RTS。辅助用户  输出2
	outb_p(0x0d,port+1);	/* enable all intrs but writes */    	// 除了写之外，开启所有总段
	(void)inb(port);	/* read data port to reset things (?) */	// 读数据口，用来进行复位操作
}

/**
 *	初始化中断控制程序和串行接口。
 */
void rs_init(void)
{
	set_intr_gate(0x24,rs1_interrupt);	// 设置中断门函数
	set_intr_gate(0x23,rs2_interrupt);
	init(tty_table[1].read_q.data);		// 初始化第一个终端
	init(tty_table[2].read_q.data);		// 初始化第二个终端
	outb(inb_p(0x21)&0xE7,0x21);		// 允许8259A得IRQ3 和IRQ4的中断控制请求
}

/*
 * This routine gets called when tty_write has put something into
 * the write_queue. It must check wheter the queue is empty, and
 * set the interrupt register accordingly
 *
 *	void _rs_write(struct tty_struct * tty);
 * 串行接口写入。 在tty_write写入数据到write_queue后调用。
 * 必须检查queue是不是空队列。 并设置相应的中断寄存器
 * 串行数据发送
 * 实际上只是开启串行发送保持寄存器已空中断标志，在UART将数据发送出去后允许中断信号
 * @param tty 终端设备
 */
void rs_write(struct tty_struct * tty)
{
	cli();		// 关中断
	// 如果队列不为空
	// 从0x3f9或0x2f9， 首先读取中断允许寄存器内容。然后添加发送保持寄存器中断允许标志（位1）。 再写回该寄存器。
	if (!EMPTY(tty->write_q))
		outb(inb_p(tty->write_q.data+1)|0x02,tty->write_q.data+1);
	sti();		// 开中断
}
