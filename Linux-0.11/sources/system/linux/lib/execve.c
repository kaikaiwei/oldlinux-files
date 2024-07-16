/*
 *  linux/lib/execve.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

// 加载并执行指定程序
// 对应int execve(const char * file, char ** argv, char ** envp)
// file: 被执行的程序文件名
// argv： 命令行参数指针数组
// envp： 环境变量指针数组
_syscall3(int,execve,const char *,file,char **,argv,char **,envp)
