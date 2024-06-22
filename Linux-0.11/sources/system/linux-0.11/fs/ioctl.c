/*
 *  linux/fs/ioctl.c
 *
 *  (C) 1991  Linus Torvalds
 * 实现输入/输出控制系统调用。 调用tty_ioctl对终端io进行控制
 */

#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <linux/sched.h>

// 终端io控制函数，chr_dev/tty_ioctl.处、
extern int tty_ioctl(int dev, int cmd, int arg);

// 定义输入输出控制函数指针类型
typedef int (*ioctl_ptr)(int dev,int cmd,int arg);

// 系统中设备种类
#define NRDEVS ((sizeof (ioctl_table))/(sizeof (ioctl_ptr)))

// ioctl操作函数指针表
static ioctl_ptr ioctl_table[]={
	NULL,		/* nodev */
	NULL,		/* /dev/mem */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	tty_ioctl,	/* /dev/ttyx */
	tty_ioctl,	/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL};		/* named pipes */
	
/**
 * 系统调用，输入输出控制函数。
 * @param fd 文件描述符
 * @param cmd 命令码
 * @param arg 参数
 * @return 0 on  success， error code on failure
 */
int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;
	int dev,mode;

	// 文件句柄超出可打开书，或者文件句柄执行的文件数据结构不存在
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	// 获取模式
	mode=filp->f_inode->i_mode;
	// 如果数不是字符设备或块设备，返回出错号
	if (!S_ISCHR(mode) && !S_ISBLK(mode))
		return -EINVAL;
	// 从文件的i节点获取设备号。 
	dev = filp->f_inode->i_zone[0];
	// 如果设备号大于系统现有设备数
	if (MAJOR(dev) >= NRDEVS)
		return -ENODEV;
	// 通过载ioctl_table中的函数，进行执行
	if (!ioctl_table[MAJOR(dev)])
		return -ENOTTY;
	return ioctl_table[MAJOR(dev)](dev,cmd,arg);
}
