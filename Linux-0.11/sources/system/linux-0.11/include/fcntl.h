#ifndef _FCNTL_H
#define _FCNTL_H

#include <stdio.h>
#include <sys/types.h>

/* open/fcntl - NOCTTY, NDELAY isn't implemented yet */
// 文件访问模式
// 文件方式模式屏蔽码
#define O_ACCMODE	00003
// 只读方式打开文件
#define O_RDONLY	   00
// 只写方式打开文件
#define O_WRONLY	   01
// 读写方式打开文件
#define O_RDWR		   02
// 如果文件不存在就创建标志
#define O_CREAT		00100	/* not fcntl */
// 独占文件使用标志
#define O_EXCL		00200	/* not fcntl */
// 不分配终端控制
#define O_NOCTTY	00400	/* not fcntl */
// 若文件已存在且数写操作，则长度截为0
#define O_TRUNC		01000	/* not fcntl */
// 以添加方式打开文件
#define O_APPEND	02000
// 非阻塞方式打开和操作文件
#define O_NONBLOCK	04000	/* not fcntl */
#define O_NDELAY	O_NONBLOCK

/* Defines for fcntl-commands. Note that currently
 * locking isn't supported, and other things aren't really
 * tested.
 */
 // fcntl命令，目前锁定命令还不支持。 
 // 复制文件句柄位最小数值的句柄
#define F_DUPFD		0	/* dup */
// 取文件句柄标志
#define F_GETFD		1	/* get f_flags */
// 设置文件句柄标志
#define F_SETFD		2	/* set f_flags */
// 取文件状态标志和访问模式
#define F_GETFL		3	/* more flags (cloexec) */
// 设置文件状态标志和访问模式
#define F_SETFL		4
// 文件锁定命令。 fcntl第三个参数指向flock结构的指针
// 返回阻止锁定的flock结构
#define F_GETLK		5	/* not implemented */
// 设置f_rdlck, f_wrlck, f_unlck锁定
#define F_SETLK		6
// 等待设置或清除锁定
#define F_SETLKW	7

/* for F_[GET|SET]FL */
// 执行exec族函数的时候，关闭文件句柄。 
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */

/* Ok, these are locking features, and aren't implemented at any
 * level. POSIX wants them.
 */
// 共享或读文件所动
#define F_RDLCK		0
// 独占或写文件锁定
#define F_WRLCK		1
// 文件解锁
#define F_UNLCK		2

/* Once again - not implemented, but ... */
struct flock {
	short l_type;		// 锁定类型
	short l_whence;		// 开始偏移， SEEk_set, seek_cur, SEEK_END
	off_t l_start;		// 阻塞锁定的开始处，相对偏移（字节数）
	off_t l_len;		// 阻塞锁定的大小
	pid_t l_pid;		// 加锁的进程id
};

// 文件创建
extern int creat(const char * filename,mode_t mode);
// 文件控制
extern int fcntl(int fildes,int cmd, ...);
// 文件打开
extern int open(const char * filename, int flags, ...);

#endif
