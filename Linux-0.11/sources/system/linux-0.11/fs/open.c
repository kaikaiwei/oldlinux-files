/*
 *  linux/fs/open.c
 *	文件操作相关的系统调用
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>

/**
 * 取文件系统信息调用函数
 */
int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;
}

/**
 * 设置文件访问和修改时间
 * @param filename 文件名称
 * @param times 访问和修改时间结构的指针
 */
int sys_utime(char * filename, struct utimbuf * times)
{
	struct m_inode * inode;
	long actime,modtime;

	// 获取文件的inode
	if (!(inode=namei(filename)))
		return -ENOENT;

	// 如果时间结构不为空
	if (times) {
		actime = get_fs_long((unsigned long *) &times->actime);
		modtime = get_fs_long((unsigned long *) &times->modtime);
	} else
		actime = modtime = CURRENT_TIME;	// 否则更新为当前时间
	
	// 设置inode时间和已更新标志
	inode->i_atime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	// 释放inode
	iput(inode);
	return 0;
}

/*
 * XXX should we use the real or effective uid?  BSD uses the real uid,
 * so as to make this call useful to setuid programs.
 * 检查对文件的访问权限
 * @param filename 是文件名
 * @param mode 屏蔽码。 R_OK,W_OK, X_OX和F_OK.
 * @return 0:请求访问允许，否则返回错误号
 */
int sys_access(const char * filename,int mode)
{
	struct m_inode * inode;
	int res, i_mode;

	// 屏蔽码由低三位组成
	mode &= 0007;
	// 获取文件对应的inode
	if (!(inode=namei(filename)))
		return -EACCES;
	// 获取文件inode的i_mode属性
	i_mode = res = inode->i_mode & 0777;
	// 释放inode
	iput(inode);
	// 当前用户
	if (current->uid == inode->i_uid)
		res >>= 6;
	else if (current->gid == inode->i_gid) // 同组用户
		res >>= 6;
	if ((res & 0007 & mode) == mode)	// 其他用户
		return 0;
	/*
	 * XXX we are doing this test last because we really should be
	 * swapping the effective with the real user id (temporarily),
	 * and then calling suser() routine.  If we do call the
	 * suser() routine, it needs to be called last. 
	 * 当前用户uid为0（超级用户）， 屏蔽码执行位数0，或文件可以被任何人访问，返回0
	 */
	if ((!current->uid) &&
	    (!(mode & 1) || (i_mode & 0111)))
		return 0;
	return -EACCES;
}

/**
 * 改变当前工作目录的系统调用
 * @param filename 新的工作目录
 * @return 0:修改完成， 否则返回错误号
 */
int sys_chdir(const char * filename)
{
	struct m_inode * inode;

	// 获取新工作目录的i节点，如果不存在则返回错误号
	if (!(inode = namei(filename)))
		return -ENOENT;
	// 如果不是目录，释放i节点，返回错误号
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	// 释放当前i节点
	iput(current->pwd);
	// 更新当前i节点
	current->pwd = inode;
	return (0);
}

/**
 * 改变根目录系统调用
 * @param filename 新的根目录
 * @return 0:操作成功，否则返回出错码
 */
int sys_chroot(const char * filename)
{
	struct m_inode * inode;

	// 获取新文件的inode，文件不存在则返回错误号
	if (!(inode=namei(filename)))
		return -ENOENT;
	// 如果不是目录，则释放inode，并返回错误号
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	// 释放当前根目录
	iput(current->root);
	// 将新的根目录设置到current->root中
	current->root = inode;
	return (0);
}

/**
 * 修改文件属性的系统调用
 * @param filename 文件名称
 * @param mode 新的文件属性
 * @return 0：成功，否则返回错误号
 */
int sys_chmod(const char * filename,int mode)
{
	struct m_inode * inode;	// 文件的i节点

	// 获取文件对应的i节点
	if (!(inode=namei(filename)))
		return -ENOENT;
	// 如果当前用户不是文件的owner或者不是超级用户，修改失败，返回错误号
	if ((current->euid != inode->i_uid) && !suser()) {
		iput(inode);
		return -EACCES;
	}
	// 修改mode
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	// 已修改标记
	inode->i_dirt = 1;
	// 释放inode
	iput(inode);
	return 0;
}

/**
 * 修改文件所属用户和所属组
 * @param filename 文件名称
 * @param uid 用户id
 * @param gid 用户组id
 */
int sys_chown(const char * filename,int uid,int gid)
{
	struct m_inode * inode;

	// 获取文件所属组id
	if (!(inode=namei(filename)))
		return -ENOENT;
	// 不是超级用户
	if (!suser()) {
		iput(inode);
		return -EACCES;
	}
	// 文件属性修改
	inode->i_uid=uid;
	inode->i_gid=gid;
	inode->i_dirt=1;
	// 释放i节点
	iput(inode);
	return 0;
}

/**
 * 文件打开系统调用
 * @param filename 文件名称
 * @param flag 读写标记。O_RDONLY, O_WRONLY,O_RDWR,O_CREATE,O_EXCL,O_APPEND等
 * @param mode 打开模式
 * @return 文件fd，或错误号，错误号小于0
 */
int sys_open(const char * filename,int flag,int mode)
{
	struct m_inode * inode;
	struct file * f;
	int i,fd;

	// 用户设置的模式和进程的品币码相与，产生许可的文件模式
	mode &= 0777 & ~current->umask;
	// 遍历进程中文件结构指针，超找一个空闲项。 如果没有空闲项，出错返回
	for(fd=0 ; fd<NR_OPEN ; fd++)
		if (!current->filp[fd])
			break;
	if (fd>=NR_OPEN)
		return -EINVAL;

	// 设置执行时关闭文件句柄位图，复位对应比特位
	current->close_on_exec &= ~(1<<fd);
	// f指向文件表数组开始的地方。搜索空间文件结构项，没有找到返回错误号
	f=0+file_table;
	for (i=0 ; i<NR_FILE ; i++,f++)
		if (!f->f_count) break;
	if (i>=NR_FILE)
		return -EINVAL;

	// 修改进程中文件结构指针对应的文件结构，并将其引用计数增加1
	(current->filp[fd]=f)->f_count++;
	// 找到对应的i节点，
	if ((i=open_namei(filename,flag,mode,&inode))<0) { // 如果没有找到
		current->filp[fd]=NULL;
		f->f_count=0;
		return i;	// 返回出错号的
	}

	// 如果是字符设备
	/* ttys are somewhat special (ttyxx major==4, tty major==5) */
	if (S_ISCHR(inode->i_mode))
		// ttyxx 下，设置当前进程的tty号位该i节点的子设备号
		// 并设置当前进程tty对应的tty表项和父进程组号等于进程的父进程组号
		if (MAJOR(inode->i_zone[0])==4) {
			if (current->leader && current->tty<0) {
				current->tty = MINOR(inode->i_zone[0]);
				tty_table[current->tty].pgrp = current->pgrp;
			}
		} else if (MAJOR(inode->i_zone[0])==5)
		// 若当前进程没有tty，说明出多，释放i节点和申请到的文件结构
			if (current->tty<0) {
				iput(inode);
				current->filp[fd]=NULL;
				f->f_count=0;
				return -EPERM;
			}
	/* Likewise with block-devices: check for floppy_change */
	// 对于块设备文件，要检查盘片是否被更换
	if (S_ISBLK(inode->i_mode))
		check_disk_change(inode->i_zone[0]);
	// 初始化文件结构。 访问属性和标志，引用计数，i节点，文件读写指针
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return (fd);
}

/**
 * 创建文件系统调用
 * @param pathname 文件路径名
 * @param mode 文件打开模式
 */
int sys_creat(const char * pathname, int mode)
{
	// 就是增加了标志的文件打开操作
	return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}

/**
 * 关闭指定fd的文件
 * @param fd 文件标号
 */
int sys_close(unsigned int fd)
{	
	struct file * filp;

	// 合法性校验
	if (fd >= NR_OPEN)
		return -EINVAL;

	// 程序关闭时，关闭的文件flags中移除fd
	current->close_on_exec &= ~(1<<fd);
	// 如果fd对应的文件不存在
	if (!(filp = current->filp[fd]))
		return -EINVAL;
	current->filp[fd] = NULL; // 置空操作

	if (filp->f_count == 0)		// 如果文件打开计数为0，表明重复关闭
		panic("Close: file count is 0");
	// 减少文件打开计数。如果为0，返回0 
	if (--filp->f_count)
		return (0);
	iput(filp->f_inode);
	return (0);
}
