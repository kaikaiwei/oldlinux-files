#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * ok, as I hadn't got any other source of information about
 * possible error numbers, I was forced to use the same numbers
 * as minix.
 * Hopefully these are posix or something. I wouldn't know (and posix
 * isn't telling me - they want $$$ for their f***ing standard).
 *
 * We don't use the _SIGN cludge of minix, so kernel returns must
 * see to the sign by themselves.
 *
 * NOTE! Remember to change strerror() if you change this file!
 */

extern int errno;

// 一般错误
#define ERROR		99
// 操作没有许可
#define EPERM		 1
// 文件或目录不存在
#define ENOENT		 2
// 指定的进程不存在
#define ESRCH		 3
// 终端的函数调用
#define EINTR		 4
// io 出错
#define EIO		 5
// 指定设备或地址不存在
#define ENXIO		 6
// 参数列表太长
#define E2BIG		 7
// 执行文件格式错误
#define ENOEXEC		 8
// 文件句柄错误
#define EBADF		 9
// 子进程不存在
#define ECHILD		10
// 资源暂时不可用
#define EAGAIN		11
// 内存不足
#define ENOMEM		12
// 没有许可权限
#define EACCES		13
// 地址错误
#define EFAULT		14
// 不是块设备文件
#define ENOTBLK		15
// 资源正忙
#define EBUSY		16
// 文件已存在
#define EEXIST		17
// 非法链接
#define EXDEV		18
// 设备不存在
#define ENODEV		19
// 不是目录文件
#define ENOTDIR		20
// 是目录文件
#define EISDIR		21
// 参数无效
#define EINVAL		22
// 系统打开文件数太多
#define ENFILE		23
// 打开文件数太多
#define EMFILE		24
// 不恰当的io控制操作
#define ENOTTY		25
// 不再使用
#define ETXTBSY		26
// 文件太大
#define EFBIG		27
// 设备已满，没有空间
#define ENOSPC		28
// 无效的文件指针重定位
#define ESPIPE		29
// 文件系统只读
#define EROFS		30
// 链接太多
#define EMLINK		31
// 管道错
#define EPIPE		32
// domain 出错
#define EDOM		33
// 结果太大
#define ERANGE		34
// 避免资源死锁
#define EDEADLK		35
// 文件名称太长
#define ENAMETOOLONG	36
// 没有锁定可用 
#define ENOLCK		37
// 没有系统调用，也就是还没有实现
#define ENOSYS		38
// 目录不空
#define ENOTEMPTY	39

#endif
