/*
 *  linux/lib/_exit.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

// 进程退出的函数调用
volatile void _exit(int exit_code)
{
	// 通过int 80 NR_exit exit_code的系统调用进行。
	__asm__("int $0x80"::"a" (__NR_exit),"b" (exit_code));
}
