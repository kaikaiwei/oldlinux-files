/*
 *  linux/lib/open.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <stdarg.h>

// 打开，并有可能创建一个文件
// filename： 文件名称
// flag： 文件打开标志
// 返回：文件描述符，如果出错，则置出错号，并返回-1
int open(const char * filename, int flag, ...)
{
	register int res;
	va_list arg;

	// 就是系统调用，
	va_start(arg,flag);
	__asm__("int $0x80"
		:"=a" (res)
		:"0" (__NR_open),"b" (filename),"c" (flag),
		"d" (va_arg(arg,int)));
	if (res>=0)
		return res;
	errno = -res;
	return -1;
}
