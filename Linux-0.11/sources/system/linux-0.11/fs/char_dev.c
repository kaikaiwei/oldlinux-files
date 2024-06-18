/*
 *  linux/fs/char_dev.c
 *  字符设备防伪控制
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <sys/types.h>

#include <linux/sched.h>
#include <linux/kernel.h>

#include <asm/segment.h>
#include <asm/io.h>			// 额定义硬件端口输入/输出宏汇编语句

// tty 读取  chr_dev/tty_io.c
extern int tty_read(unsigned minor,char * buf,int count);
// tty 写入  chr_dev/tty_io.c
extern int tty_write(unsigned minor,char * buf,int count);

/**
 * 定义字符设备读写函数指针类型
 * @param rw 读写
 * @param minor 终端子设备号
 * @param buf 缓冲区
 * @param count 读写字节数
 * @param pos 读写操作当前指针。对于终端，该指针无意义
 * @return 实际返回字节数
 */
typedef (*crw_ptr)(int rw,unsigned minor,char * buf,int count,off_t * pos);

/**
 * 串口终端读写操作函数
 * @param rw 读写
 * @param minor 终端子设备号
 * @param buf 缓冲区
 * @param count 读写字节数
 * @param pos 读写操作当前指针。对于终端，该指针无意义
 * @return 实际返回字节数
 */
static int rw_ttyx(int rw,unsigned minor,char * buf,int count,off_t * pos)
{
	return ((rw==READ)?tty_read(minor,buf,count):
		tty_write(minor,buf,count));
}

/**
 * 终端读写操作函数。 增加了对进程是否有控制终端的检测
 * @param rw 读写
 * @param minor 终端子设备号
 * @param buf 缓冲区
 * @param count 读写字节数
 * @param pos 读写操作当前指针。对于终端，该指针无意义
 * @return 实际返回字节数
 */
static int rw_tty(int rw,unsigned minor,char * buf,int count, off_t * pos)
{
	// 如果进程没有对应的控制终端，则返回出错号
	if (current->tty<0)
		return -EPERM;
	return rw_ttyx(rw,current->tty,buf,count,pos);
}

// 内存数据读写，未实现
static int rw_ram(int rw,char * buf, int count, off_t *pos)
{
	return -EIO;
}

// 内存数据读写操作函数。未实现
static int rw_mem(int rw,char * buf, int count, off_t * pos)
{
	return -EIO;
}

// 内核数据区读写函数
static int rw_kmem(int rw,char * buf, int count, off_t * pos)
{
	return -EIO;
}

/**
 * 端口读写操作函数
 * @param rw 读写
 * @param buf 数据缓冲区
 * @param count 读写字节长度
 * @param pos 端口地址
 * @return 实际读写的字节数
 */ 
static int rw_port(int rw,char * buf, int count, off_t * pos)
{
	int i=*pos;

	// 总共有65535个端口
	while (count-->0 && i<65536) {
		// 读命令，从从端口读取一个字节并放到用户缓冲区中
		if (rw==READ)
			put_fs_byte(inb(i),buf++);
		else
			// 写命令，从buf中读取一个字节写入端口
			outb(get_fs_byte(buf++),i);
		i++;	// 前移一个端口
	}
	i -= *pos;	// 计算读写的字节数
	*pos += i;	
	return i;	// 返回读写的字节数
}

/**
 * 内存读写操作函数
 * @param rw 读写标志
 * @param minor 子设备号
 * @param buf  用户缓冲区
 * @param count  读写长度
 * @param pos
 * @return 实际读写的字节数
 */
static int rw_memory(int rw, unsigned minor, char * buf, int count, off_t * pos)
{
	// 根据不同的子设备号，调用不同的内存读写函数。
	switch(minor) {
		case 0:
			return rw_ram(rw,buf,count,pos);
		case 1:
			return rw_mem(rw,buf,count,pos);
		case 2:
			return rw_kmem(rw,buf,count,pos);
		case 3:
			return (rw==READ)?0:count;	/* rw_null */
		case 4:
			return rw_port(rw,buf,count,pos);
		default:
			return -EIO;
	}
}

// 定义系统中的设备种类
#define NRDEVS ((sizeof (crw_table))/(sizeof (crw_ptr)))

// 字符设备读写函数指针表
static crw_ptr crw_table[]={
	NULL,		/* nodev */			// 无设备
	rw_memory,	/* /dev/mem etc */	// /dev/mem
	NULL,		/* /dev/fd */		// /dev/fd 软驱
	NULL,		/* /dev/hd */		// /dev/hd 硬盘
	rw_ttyx,	/* /dev/ttyx */		// /dev/ttyx 串口终端
	rw_tty,		/* /dev/tty */		// /dev/tty 终端
	NULL,		/* /dev/lp */		// /dev/lp 打印机
	NULL};		/* unnamed pipes */	// 未命名管道

// 字符设备读写操作函数
int rw_char(int rw,int dev, char * buf, int count, off_t * pos)
{
	crw_ptr call_addr;

	// 主设备好超出系统设备数，则返回出错号
	if (MAJOR(dev)>=NRDEVS)
		return -ENODEV;
	// 若该设备没有对应的读写函数，则返回出错号
	if (!(call_addr=crw_table[MAJOR(dev)]))
		return -ENODEV;
	// 调用对应的读写操作函数，并返回实际读写的字节数
	return call_addr(rw,MINOR(dev),buf,count,pos);
}
