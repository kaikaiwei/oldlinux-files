/*
 *  linux/fs/fcntl.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <fcntl.h>
#include <sys/stat.h>

extern int sys_close(int fd); // 文件关闭系统调用

/**
 * 复制文件句柄
 * @param fd 要复制的文件句柄
 * @param arg 执行新文件句柄的最小数值
 * @return 新的文件句柄或出错号
 */
static int dupfd(unsigned int fd, unsigned int arg)
{
	// 句柄大于程序最多打开文件数，或者句柄的文件结构不存在
	if (fd >= NR_OPEN || !current->filp[fd])
		return -EBADF;
	// 新的最小句柄大于程序对多打开文件数
	if (arg >= NR_OPEN)
		return -EINVAL;
	// 遍历查找arg， 找到空闲项停下
	while (arg < NR_OPEN)
		if (current->filp[arg])
			arg++;
		else
			break;
	if (arg >= NR_OPEN)
		return -EMFILE;
	// 增加程序关闭时的文件关闭标志
	current->close_on_exec &= ~(1<<arg);
	// 增加文件引用计数，新文件使用旧文件的信息
	(current->filp[arg] = current->filp[fd])->f_count++;
	return arg;
}

/**
 * 系统调用，复制文件句柄，指定最小句柄号
 */
int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	sys_close(newfd);
	return dupfd(oldfd,newfd);
}

/**
 * 系统调用，复制文件句柄，最小句柄号指定为0
 */
int sys_dup(unsigned int fildes)
{
	return dupfd(fildes,0);
}

/**
 * 文件控制系统调用函数
 * @param fd 文件句柄
 * @param cmd 操作命令，见fcntl.h
 * @param arg 参数
 */
int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;

	// 句柄大于程序最多打开文件数，或者句柄的文件结构不存在
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:
			return dupfd(fd,arg);	// 复制文件句柄
		case F_GETFD:
			return (current->close_on_exec>>fd)&1; // 获取fd
		case F_SETFD:	// 设置文件句柄
			if (arg&1)
				current->close_on_exec |= (1<<fd);
			else
				current->close_on_exec &= ~(1<<fd);
			return 0;
		case F_GETFL:			// 获取文件flag
			return filp->f_flags;
		case F_SETFL:			// 设置文件flag
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		case F_GETLK:	case F_SETLK:	case F_SETLKW:
			return -1;
		default:
			return -1;
	}
}
