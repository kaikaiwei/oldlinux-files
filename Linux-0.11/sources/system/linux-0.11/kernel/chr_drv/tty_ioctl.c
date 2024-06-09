/*
 *  linux/kernel/chr_drv/tty_ioctl.c
 *
 *  (C) 1991  Linus Torvalds
 * 字符设备控制操作
 */

#include <errno.h>
#include <termios.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/tty.h>

#include <asm/io.h>
#include <asm/segment.h>
#include <asm/system.h>

// 波特率因子数组。 除数数组
static unsigned short quotient[] = {
	0, 2304, 1536, 1047, 857,
	768, 576, 384, 192, 96,
	64, 48, 24, 12, 6, 3
};

/**
 * 修改传输速率。
 * @param tty 终端数据结构
 * 在除数所存标志DLAB(线路控制寄存器位7)置位的情况下，通过端口0x3f8和0x2f8向UART
 * 分别写入波特率因子低字节和高字节
 */
static void change_speed(struct tty_struct * tty)
{
	unsigned short port,quot;

	// 端口为0，不合法。
	if (!(port = tty->read_q.data))
		return;
	// 从波特率因子数组中获取对应的波特率因子值
	quot = quotient[tty->termios.c_cflag & CBAUD];
	cli();			// 关中断
	outb_p(0x80,port+3);		/* set DLAB */		// 设置DLAB
	outb_p(quot & 0xff,port);	/* LS of divisor */	// 写入低字节到port中
	outb_p(quot >> 8,port+1);	/* MS of divisor */	// 写入高字节到port中
	outb(0x03,port+3);		/* reset DLAB */		// 复位DLAB
	sti();			// 开中断
}

/**
 * 刷新tty缓冲队列。
 * @param queue 指定的缓冲队列指针
 */
static void flush(struct tty_queue * queue)
{
	cli();
	queue->head = queue->tail; // 头指针等于尾指针，清空缓冲区
	sti();
}

/**
 * 等待字符发送出去
 */
static void wait_until_sent(struct tty_struct * tty)
{
	/* do nothing - not implemented */
}

/**
 * 发送BREAK控制符
 */
static void send_break(struct tty_struct * tty)
{
	/* do nothing - not implemented */
}

/**
 * 取终端termios结构信息
 * @param tty： 终端tty结构指针
 * @param termios 用户数据区termios结构缓冲区指针
 */
static int get_termios(struct tty_struct * tty, struct termios * termios)
{
	int i;

	// 验证是否满足termios保存的需求
	verify_area(termios, sizeof (*termios));
	// 逐字节放到用户缓冲区
	for (i=0 ; i< (sizeof (*termios)) ; i++)
		put_fs_byte( ((char *)&tty->termios)[i] , i+(char *)termios );
	return 0;
}

/**
 * 设置termios结构
 * @param tty ： 中断tty 结构指针
 * @param termios 用于数据区termios结构缓冲区指针。
 * @return 0
 */
static int set_termios(struct tty_struct * tty, struct termios * termios)
{
	int i;
	// 逐字节的拷贝到内核缓冲区中
	for (i=0 ; i< (sizeof (*termios)) ; i++)
		((char *)&tty->termios)[i]=get_fs_byte(i+(char *)termios);
	// 修改波特率因子
	change_speed(tty);
	return 0;
}

/**
 * 读取termios中的信息
 * @param tty 指定中断的额tty结构指针
 * @param termio 用户数据区。termio数据指针。
 * @return 0
 */
static int get_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	verify_area(termio, sizeof (*termio));
	tmp_termio.c_iflag = tty->termios.c_iflag;
	tmp_termio.c_oflag = tty->termios.c_oflag;
	tmp_termio.c_cflag = tty->termios.c_cflag;
	tmp_termio.c_lflag = tty->termios.c_lflag;
	tmp_termio.c_line = tty->termios.c_line;
	for(i=0 ; i < NCC ; i++)
		tmp_termio.c_cc[i] = tty->termios.c_cc[i];
	for (i=0 ; i< (sizeof (*termio)) ; i++)
		put_fs_byte( ((char *)&tmp_termio)[i] , i+(char *)termio );
	return 0;
}

/*
 * This only works as the 386 is low-byt-first
 * 设置中断termio结构信息
 * 尽在386低字节在前的方式下可用
 */
static int set_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	for (i=0 ; i< (sizeof (*termio)) ; i++)
		((char *)&tmp_termio)[i]=get_fs_byte(i+(char *)termio);
	*(unsigned short *)&tty->termios.c_iflag = tmp_termio.c_iflag;
	*(unsigned short *)&tty->termios.c_oflag = tmp_termio.c_oflag;
	*(unsigned short *)&tty->termios.c_cflag = tmp_termio.c_cflag;
	*(unsigned short *)&tty->termios.c_lflag = tmp_termio.c_lflag;
	tty->termios.c_line = tmp_termio.c_line;
	for(i=0 ; i < NCC ; i++)
		tty->termios.c_cc[i] = tmp_termio.c_cc[i];
	change_speed(tty);
	return 0;
}

/**
 * tty中断设备的ioctl函数
 * @param dev 子设备号
 * @param cmd 命令
 * @param arg 操作参数指针
 */
int tty_ioctl(int dev, int cmd, int arg)
{
	struct tty_struct * tty;

	// tty 的主设备号是5（tty终端），则进程的tty字段就是子设备号。如果tty的子设备号是负的，
	// 表示该进程没有控制终端，也就不能发出ioctl调用
	if (MAJOR(dev) == 5) {
		dev=current->tty;
		if (dev<0)
			panic("tty_ioctl: dev<0");
	} else
		dev=MINOR(dev);
	// 得到tty数据结构信息
	tty = dev + tty_table;
	// 命令处理，然后就是各种调用了。
	switch (cmd) {
		case TCGETS:
			return get_termios(tty,(struct termios *) arg); 
		case TCSETSF:
			flush(&tty->read_q); /* fallthrough */
		case TCSETSW:
			wait_until_sent(tty); /* fallthrough */
		case TCSETS:
			return set_termios(tty,(struct termios *) arg);
		case TCGETA:
			return get_termio(tty,(struct termio *) arg);
		case TCSETAF:
			flush(&tty->read_q); /* fallthrough */
		case TCSETAW:
			wait_until_sent(tty); /* fallthrough */
		case TCSETA:
			return set_termio(tty,(struct termio *) arg);
		case TCSBRK:
			if (!arg) {
				wait_until_sent(tty);
				send_break(tty);
			}
			return 0;
		case TCXONC:
			return -EINVAL; /* not implemented */
		case TCFLSH:
			if (arg==0)
				flush(&tty->read_q);
			else if (arg==1)
				flush(&tty->write_q);
			else if (arg==2) {
				flush(&tty->read_q);
				flush(&tty->write_q);
			} else
				return -EINVAL;
			return 0;
		case TIOCEXCL:
			return -EINVAL; /* not implemented */
		case TIOCNXCL:
			return -EINVAL; /* not implemented */
		case TIOCSCTTY:
			return -EINVAL; /* set controlling term NI */
		case TIOCGPGRP:
			verify_area((void *) arg,4);
			put_fs_long(tty->pgrp,(unsigned long *) arg);
			return 0;
		case TIOCSPGRP:
			tty->pgrp=get_fs_long((unsigned long *) arg);
			return 0;
		case TIOCOUTQ:
			verify_area((void *) arg,4);
			put_fs_long(CHARS(tty->write_q),(unsigned long *) arg);
			return 0;
		case TIOCINQ:
			verify_area((void *) arg,4);
			put_fs_long(CHARS(tty->secondary),
				(unsigned long *) arg);
			return 0;
		case TIOCSTI:
			return -EINVAL; /* not implemented */
		case TIOCGWINSZ:
			return -EINVAL; /* not implemented */
		case TIOCSWINSZ:
			return -EINVAL; /* not implemented */
		case TIOCMGET:
			return -EINVAL; /* not implemented */
		case TIOCMBIS:
			return -EINVAL; /* not implemented */
		case TIOCMBIC:
			return -EINVAL; /* not implemented */
		case TIOCMSET:
			return -EINVAL; /* not implemented */
		case TIOCGSOFTCAR:
			return -EINVAL; /* not implemented */
		case TIOCSSOFTCAR:
			return -EINVAL; /* not implemented */
		default:
			return -EINVAL;
	}
}
