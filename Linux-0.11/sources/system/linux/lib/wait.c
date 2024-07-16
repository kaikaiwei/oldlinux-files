/*
 *  linux/lib/wait.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <sys/wait.h>

/**
 * 等待进程终止系统调用函数
 * @param pid 等待被终止的进程id
 * @param wait_stat 存放状态信息
 * @param options  WNOHANG/WUNTRACED
 */
_syscall3(pid_t,waitpid,pid_t,pid,int *,wait_stat,int,options)

pid_t wait(int * wait_stat)
{
	return waitpid(-1,wait_stat,0);
}
