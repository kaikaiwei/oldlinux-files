/*
 *  linux/fs/file_dev.c
 * 
 *  通过指定路径名
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <fcntl.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

// 取二者中最小值的宏
#define MIN(a,b) (((a)<(b))?(a):(b))
// 取二者中最大值的宏
#define MAX(a,b) (((a)>(b))?(a):(b))

/**
 * 文件读取
 * @param inode 文件i节点
 * @param filep 文件数据结构指针
 * @param buf   读取到的缓冲区
 * @param count 读取多少个字节
 * @return 最终读取的结果
 */
int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left,chars,nr;
	struct buffer_head * bh;

	// 如果读取0字节，直接返回
	if ((left=count)<=0)
		return 0;

	// left表示剩余要读取的字节
	while (left) {
		// 根据i节点信息取文件数据块block在设备上对应的逻辑块号， 
		// nr 表示文件的逻辑块号
		if (nr = bmap(inode,(filp->f_pos)/BLOCK_SIZE)) {
			// 如果nr不为0， 读取nr的内容到高速缓冲区。 如果读取失败，返回
			if (!(bh=bread(inode->i_dev,nr)))
				break;
		} else
			bh = NULL;
		
		// nr这里表示块内偏移量
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN( BLOCK_SIZE-nr , left ); // chars表示要读取的内容长度，取块中剩余字节和剩下要读取字节数的最小值
		// 修改文件当前读取位置
		filp->f_pos += chars;
		// 修改剩余要读取内容
		left -= chars;
		if (bh) {
			// p指向数据位置
			char * p = nr + bh->b_data;
			// 循环读取chars个到buf中
			while (chars-->0)
				put_fs_byte(*(p++),buf++);
			brelse(bh);
		} else {
			// 往用户缓冲区填入0字节
			while (chars-->0)
				put_fs_byte(0,buf++);
		}
	}
	inode->i_atime = CURRENT_TIME;	// 修改访问时间
	// 返回已经读取的内容
	return (count-left)?(count-left):-ERROR;
}

/**
 * 文件写入操作
 * @param inode 文件i节点
 * @param filep 文件数据结构指针
 * @param buf	数据来源
 * @param count	写入长度
 * @return 最终写入的长度
 */
int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	off_t pos;	文件偏移量
	int block,c;
	struct buffer_head * bh;
	char * p;
	int i=0;	// 当前写入字节数

	/*
	* ok, append may not work when many processes are writing at the same time
	* but so what. That way leads to madness anyway.
	*/
	// 如果是追加模式，当前位置是文件长度，否则是文件当前位置。
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;

	// 当前已经写入字节，小于
	while (i<count) {
		// 创建块，如果不存在则退出循环
		if (!(block = create_block(inode,pos/BLOCK_SIZE)))
			break;

		// 读取逻辑块数据到高速缓冲区
		if (!(bh=bread(inode->i_dev,block)))
			break;

		// 找到写入位置。
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;

		// 置已修改标志
		bh->b_dirt = 1;
		c = BLOCK_SIZE-c;	// 高速缓冲区剩余字节数

		// 如果该block可以完整写入。 那么更新
		if (c > count-i) c = count-i;
		// 更新文件位置
		pos += c;

		// 如果当前文件读写指针大于文件大小
		if (pos > inode->i_size) {
			inode->i_size = pos;	// 更新文件大小
			inode->i_dirt = 1;		// 已修改标志
		}
		
		// 增加写入字节数
		i += c;
		// 写入字节
		while (c-->0)
			*(p++) = get_fs_byte(buf++);
		// 释放高速缓冲区
		brelse(bh);
	}

	// 文件修改时间
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i?i:-1);
}
