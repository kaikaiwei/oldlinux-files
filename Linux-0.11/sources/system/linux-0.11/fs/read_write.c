/*
 *  linux/fs/read_write.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/segment.h>

/** 外部真实实现的函数 */
extern int rw_char(int rw,int dev, char * buf, int count, off_t * pos);
extern int read_pipe(struct m_inode * inode, char * buf, int count);
extern int write_pipe(struct m_inode * inode, char * buf, int count);
extern int block_read(int dev, off_t * pos, char * buf, int count);
extern int block_write(int dev, off_t * pos, char * buf, int count);
extern int file_read(struct m_inode * inode, struct file * filp,
		char * buf, int count);
extern int file_write(struct m_inode * inode, struct file * filp,
		char * buf, int count);

/**
 * 重定位文件读写指针系统调用
 * @param fd 文件id
 * @param offset 偏移量
 * @param origin 起始位置。0:文件开始。1:当前位置，2:从文件结尾处。
 */
int sys_lseek(unsigned int fd,off_t offset, int origin)
{
	struct file * file;
	int tmp;

	// 检查fd是否合法，拿到文件数据结构指针， 检查文件inode数据结构。判断是否可以
	if (fd >= NR_OPEN || !(file=current->filp[fd]) || !(file->f_inode)
	   || !IS_SEEKABLE(MAJOR(file->f_inode->i_dev)))
		return -EBADF;
	// pipe类型，不支持
	if (file->f_inode->i_pipe)
		return -ESPIPE;
	// 原来的位置
	switch (origin) {
		case 0:
			if (offset<0) return -EINVAL;
			file->f_pos=offset;
			break;
		case 1:
			if (file->f_pos+offset<0) return -EINVAL;
			file->f_pos += offset;
			break;
		case 2:
			if ((tmp=file->f_inode->i_size+offset) < 0)
				return -EINVAL;
			file->f_pos = tmp;
			break;
		default:
			return -EINVAL;
	}
	return file->f_pos;
}

/**
 * 文件读取系统调用
 * @param fd 文件id
 * @param buf 用户缓冲区
 * @param count 读写字节数
 * @return 实际读取字节数
 */
int sys_read(unsigned int fd,char * buf,int count)
{
	struct file * file;
	struct m_inode * inode;

	// fd 不合法， 写入字节数小于0， 文件数据结构指针为空。返回错误号
	if (fd>=NR_OPEN || count<0 || !(file=current->filp[fd]))
		return -EINVAL;
	// 如果count为0， 直接返回0
	if (!count)
		return 0;
	// 验证用户缓冲区是否有足够的读写字节，避免内存访问越界
	verify_area(buf,count);
	// 得到文件的inode
	inode = file->f_inode;

	// 处理pipe函数
	if (inode->i_pipe)
		return (file->f_mode&1)?read_pipe(inode,buf,count):-EIO;

	// 如果是字符设备
	if (S_ISCHR(inode->i_mode))
		return rw_char(READ,inode->i_zone[0],buf,count,&file->f_pos);

	// 如果是块设备
	if (S_ISBLK(inode->i_mode))
		return block_read(inode->i_zone[0],&file->f_pos,buf,count);

	// 如果是目录或常规文件
	if (S_ISDIR(inode->i_mode) || S_ISREG(inode->i_mode)) {
		// 验证文件大小是否合适
		if (count+file->f_pos > inode->i_size)
			count = inode->i_size - file->f_pos; // 更新为文件剩余大小
		if (count<=0)
			return 0;
		// 返回文件阅读
		return file_read(inode,file,buf,count);
	}
	printk("(Read)inode->i_mode=%06o\n\r",inode->i_mode);
	return -EINVAL;
}

/**
 * 文件写入系统调用
 * @param fd 文件id
 * @param buf 用户缓冲区
 * @param count 读写字节数
 * @return 实际写入字节数
 */
int sys_write(unsigned int fd,char * buf,int count)
{
	struct file * file;
	struct m_inode * inode;
	
	// fd 不合法， 写入字节数小于0， 文件数据结构指针为空。返回错误号
	if (fd>=NR_OPEN || count <0 || !(file=current->filp[fd]))
		return -EINVAL;
	
	// 如果count 为0， 返回0
	if (!count)
		return 0;

	// 得到文件的inode
	inode=file->f_inode;
	// 管道写入
	if (inode->i_pipe)
		return (file->f_mode&2)?write_pipe(inode,buf,count):-EIO;
	
	// 字符设备写入
	if (S_ISCHR(inode->i_mode))
		return rw_char(WRITE,inode->i_zone[0],buf,count,&file->f_pos);

	// 块设备写入
	if (S_ISBLK(inode->i_mode))
		return block_write(inode->i_zone[0],&file->f_pos,buf,count);

	// 普通文件读写
	if (S_ISREG(inode->i_mode))
		return file_write(inode,file,buf,count);
	printk("(Write)inode->i_mode=%06o\n\r",inode->i_mode);
	return -EINVAL;
}
