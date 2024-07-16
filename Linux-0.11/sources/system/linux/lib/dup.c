/*
 *  linux/lib/dup.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

// 复制文件描述符函数
// 对应int dup(int fd); 直接调用系统中断 int 0x80 ，参数是__NR_dup. fd是文件描述符
_syscall1(int,dup,int,fd)
