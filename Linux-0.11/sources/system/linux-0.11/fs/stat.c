/*
 *  linux/fs/stat.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

/**
 * 复制文件状态信息
 * @param inode 文件i节点
 * @param statbuf 复制内容到。 复制到用户缓冲区中
 */
static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	// 验证satbuf可以放入对应大小的内容
	verify_area(statbuf,sizeof (* statbuf));
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_num;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlinks;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_zone[0];
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	// 复制到用户缓冲区中
	for (i=0 ; i<sizeof (tmp) ; i++)
		put_fs_byte(((char *) &tmp)[i],&((char *) statbuf)[i]);
}

/**
 * 通过文件名获取文件信息
 * @param filename 文件名称
 * @param statbuf 数据存放位置
 */
int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	// 获取i节点
	if (!(inode=namei(filename)))
		return -ENOENT;
	// 拷贝数据到statbuf中
	cp_stat(inode,statbuf);
	// 释放i节点
	iput(inode);
	return 0;
}

/**
 * 通过fd获取stat信息
 * @param 文件fd
 */
int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	// 校验fd的合法性。 超过了文件打开数组长度， 当前进程的文件打开表的文件数据结构为空。或者文件的inode为空
	if (fd >= NR_OPEN || !(f=current->filp[fd]) || !(inode=f->f_inode))
		return -EBADF;
	// 通过i节点进行拷贝
	cp_stat(inode,statbuf);
	return 0;
}
