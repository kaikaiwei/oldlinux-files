/*
 *  linux/fs/pipe.c
 *

 *  在初始化管道时， 管道i节点的i_size字段中被设置为执行管道缓冲区的指针
 *  管道数据头部指针放在i_zone【0】中。管道数据尾部指针放到i_zone【1】中。
 *  对读管道操作，数据是从管道尾独处，并使管道尾指针前移读取字节数个位置；
 *  对于写操作，数据从管道头部写入，并使管道头指针前移写入字节个数个位置。
 *  (C) 1991  Linus Torvalds
 */

#include <signal.h>

#include <linux/sched.h>
#include <linux/mm.h>	/* for get_free_page */
#include <asm/segment.h>

/**
 * 管道读取， 如果没有数据，则唤醒写管道进程
 * @param inode 管道的i节点
 * @param buf 读入缓冲区
 * @param counnt 读如字节数
 * @return 读取的字节数
 */
int read_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, read = 0;

	while (count>0) {
		// 如果管道中数据为0
		while (!(size=PIPE_SIZE(*inode))) {
			// 唤醒写入进程
			wake_up(&inode->i_wait);
			// 如果写入进程不存在，返回已经读如的字节数
			if (inode->i_count != 2) /* are there any writers? */
				return read;
			// 睡眠等待
			sleep_on(&inode->i_wait);
		}

		// 管道中有效数据长度（已经写入的）
		chars = PAGE_SIZE-PIPE_TAIL(*inode);
		// 管道中有效数据长度大于要去读字节
		if (chars > count)
			chars = count;
		// 管道中有效数据长度大于数据长度
		if (chars > size)
			chars = size;
		// 读如chars个字节
		count -= chars;
		read += chars;
		size = PIPE_TAIL(*inode);
		PIPE_TAIL(*inode) += chars;
		PIPE_TAIL(*inode) &= (PAGE_SIZE-1);
		// 逐字节拷贝到buf中
		while (chars-->0)
			put_fs_byte(((char *)inode->i_size)[size++],buf++);
	}
	// 唤醒等待进程
	wake_up(&inode->i_wait);
	return read;
}

/**
 * 管道写函数
 * @param inode 写入节点
 * @param buf 待写入输入缓冲区
 * @param count 写入字节数
 * @return 真实写入字节数
 */
int write_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, written = 0;

	// 如果待写入字节数大于0
	while (count>0) {
		// 当缓冲区中没有要剩余空间时
		while (!(size=(PAGE_SIZE-1)-PIPE_SIZE(*inode))) {
			// 唤醒读进程
			wake_up(&inode->i_wait);
			// 如果没有读进程
			if (inode->i_count != 2) { /* no readers */
				// 添加信号量。
				current->signal |= (1<<(SIGPIPE-1));
				return written?written:-1;
			}
			// 睡眠等待
			sleep_on(&inode->i_wait);
		}

		// 页面中的剩余课写入空间大小
		chars = PAGE_SIZE-PIPE_HEAD(*inode);
		// 剩余空间超过待写入字节
		if (chars > count)
			chars = count;
		// 剩余空间超过到当前空闲空间的长度
		if (chars > size)
			chars = size;
		// chars这里表示本次要写入的字节数
		count -= chars;
		written += chars;
		// size指向管道数据头部
		size = PIPE_HEAD(*inode);
		// 调整头部指针
		PIPE_HEAD(*inode) += chars;
		PIPE_HEAD(*inode) &= (PAGE_SIZE-1);
		// 字节写入
		while (chars-->0)
			((char *)inode->i_size)[size++]=get_fs_byte(buf++);
	}
	// 唤醒等待进程
	wake_up(&inode->i_wait);
	return written;
}

/**
 * 创建管道系统调用
 * 在filds指定的数组中，创建一对文件描述符。下标0用于读数据，下标1用于写数据
 * @param fildes 数组。
 */
int sys_pipe(unsigned long * fildes)
{
	struct m_inode * inode;
	struct file * f[2];
	int fd[2];
	int i,j;

	// 从文件系统中取出两个空闲项
	j=0;
	for(i=0;j<2 && i<NR_FILE;i++)
		if (!file_table[i].f_count)
			(f[j++]=i+file_table)->f_count++;
	// 只有一个空闲项
	if (j==1)
		f[0]->f_count=0;
	// 没有找到两个空闲项
	if (j<2)
		return -1;

	// 针对找到的文件结构项，添加到当前程序的句柄中
	j=0;
	for(i=0;j<2 && i<NR_OPEN;i++)
		if (!current->filp[i]) {
			current->filp[ fd[j]=i ] = f[j];
			j++;
		}
	// 当前程序的句柄置添加成功一个
	if (j==1)
		current->filp[fd[0]]=NULL;
	// 没有添加两个句柄
	if (j<2) {
		f[0]->f_count=f[1]->f_count=0;
		return -1;
	}

	// 申请i节点，如果失败则清空当前程序文件句柄。
	if (!(inode=get_pipe_inode())) {
		current->filp[fd[0]] =
			current->filp[fd[1]] = NULL;
		f[0]->f_count = f[1]->f_count = 0;
		return -1;
	}
	// 初始化数组的inode
	f[0]->f_inode = f[1]->f_inode = inode;
	f[0]->f_pos = f[1]->f_pos = 0;
	f[0]->f_mode = 1;		/* read */
	f[1]->f_mode = 2;		/* write */
	// 放入到用户空间数组中
	put_fs_long(fd[0],0+fildes);
	put_fs_long(fd[1],1+fildes);
	return 0;
}
