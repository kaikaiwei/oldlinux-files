#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>

// 取低字节
#define _LOW(v)		( (v) & 0377)
// 取高字节
#define _HIGH(v)	( ((v) >> 8) & 0377)

/* options for waitpid, WUNTRACED not supported */
// waitpid选项， WUNTRACED没有被支持
// WNOHANG 没有状态页不要挂起，并立即返回
#define WNOHANG		1
#define WUNTRACED	2

// 如果子进程正常退出，则为真
#define WIFEXITED(s)	(!((s)&0xFF)
// 如果子进程正在停止，则为真
#define WIFSTOPPED(s)	(((s)&0xFF)==0x7F)
// 返回退出状态
#define WEXITSTATUS(s)	(((s)>>8)&0xFF)
// 返回导致进程终止的信号值（信号量）
#define WTERMSIG(s)	((s)&0x7F)
// 返回导致进程终止的信号值（信号量）
#define WSTOPSIG(s)	(((s)>>8)&0xFF)
// 如果由于未捕获到信号而导致子进程退出，则为真。
#define WIFSIGNALED(s)	(((unsigned int)(s)-1 & 0xFFFF) < 0xFF)

// 允许进程获取器子进程之一的状态信息。
pid_t wait(int *stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

#endif
