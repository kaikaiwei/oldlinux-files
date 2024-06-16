/*
 *  linux/fs/block_dev.c
 *
 *  (C) 1991  Linus Torvalds
 *  主要实现块设备的read/write系统调用
 */

#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

/**
 * 块设备写函数系统调用
 * 对于内核来说，写操作是向高速缓冲区中写入数据，什么时候数据最终写入到设备是有高速缓冲管理程序决定的。
 * 
 * @param dev 设备号
 * @param pos 写的位置信息（偏移）
 * @param buf 写的内容
 * @param count 写入字节数
 * @return 写入的长度
 */
int block_write(int dev, long * pos, char * buf, int count)
{
	int block = *pos >> BLOCK_SIZE_BITS;	// 写入位置所在的块号
	int offset = *pos & (BLOCK_SIZE-1);		// 块内偏移
	int chars;
	int written = 0;			// 已经写入字节数
	struct buffer_head * bh;
	register char * p;			// 寄存器

	// 待写入数据大于0
	while (count>0) {
		chars = BLOCK_SIZE - offset;  // 计算该块中可写入的字节数

		// 可写入字节数大于待写入数据长度， 只需要写入count个字节
		if (chars > count)
			chars=count;

		// 可写入字节数等于块大小
		if (chars == BLOCK_SIZE)
			bh = getblk(dev,block);	// 获取对应的高速缓冲块
		else
			bh = breada(dev,block,block+1,block+2,-1);	// 欲读多块到高速缓冲区中
		block++;	// 增加块号
		
		// 如果高速缓冲区为空。 返回写入字节数
		if (!bh)
			return written?written:-EIO;

		// 写入的位置
		p = offset + bh->b_data;
		offset = 0;
		// 更新文件位置
		*pos += chars;
		written += chars;	// 增加写入的字节
		count -= chars;		// 减少待写入的字节
		
		// 从buf中获取写入内容，写入到p中
		while (chars-->0)
			*(p++) = get_fs_byte(buf++);
		bh->b_dirt = 1;		// 已修改标记
		brelse(bh);
	}
	return written;
}

/**
 * 从块设备读取指定的内容
 * @param dev 设备号
 * @param pos 文件中的读取位置
 * @param buf 读取到的位置
 * @param count 读取的字节数
 * @return 读取的内容长度
 */
int block_read(int dev, unsigned long * pos, char * buf, int count)
{
	int block = *pos >> BLOCK_SIZE_BITS;	// 读取位置所在的块号
	int offset = *pos & (BLOCK_SIZE-1);		// 块内的偏移量
	int chars;						// 块内需要读取多少字节
	int read = 0;					// 已经读取的长度
	struct buffer_head * bh;
	register char * p;

	// 需要读取内容的长度大于0
	while (count>0) {
		chars = BLOCK_SIZE-offset;	// 块内剩余没有读的字节数

		// 如果块内剩余没有读取的大于要读取的
		if (chars > count)
			chars = count;

		// 预读
		if (!(bh = breada(dev,block,block+1,block+2,-1)))
			return read?read:-EIO;

		// 增加块号
		block++;
		// 读取的指针
		p = offset + bh->b_data;
		offset = 0;
		// 更新文件位置
		*pos += chars;
		read += chars;

		// 待读取字节数减少
		count -= chars;
		// 拷贝数据到用户缓冲区buf中
		while (chars-->0)
			put_fs_byte(*(p++),buf++);
		// 释放高速缓冲区
		brelse(bh);
	}
	return read;
}
