#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>

typedef int sig_atomic_t;
typedef unsigned int sigset_t;		/* 32 bits */

// 信号种类，共计32种
#define _NSIG             32
#define NSIG		_NSIG

// 挂断控制终端或进程
#define SIGHUP		 1
// 来自键盘的中断
#define SIGINT		 2
// 来自键盘的退出
#define SIGQUIT		 3
// 非法指令
#define SIGILL		 4
// 跟踪断点
#define SIGTRAP		 5
// 异常结束
#define SIGABRT		 6
// 异常结束
#define SIGIOT		 6
// 没有使用
#define SIGUNUSED	 7
// 协处理器出错
#define SIGFPE		 8
// 强迫进程中止
#define SIGKILL		 9
// 用户信号1，进程可使用
#define SIGUSR1		10
// 无效内存引用
#define SIGSEGV		11
// 用户信号2， 进程可使用
#define SIGUSR2		12
// 管道写出错，无读者
#define SIGPIPE		13
// 实时定时器报警
#define SIGALRM		14
// 进程终止
#define SIGTERM		15
// 栈出错， 协处理器 stack fault
#define SIGSTKFLT	16
// 子进程停止或被中止
#define SIGCHLD		17
// 恢复进程继续执行， continue
#define SIGCONT		18
// 停止进程的执行
#define SIGSTOP		19
// tty 发出停止进程，可忽略
#define SIGTSTP		20
// 后台进程请求进入
#define SIGTTIN		21
// 后台进程请求退出
#define SIGTTOU		22

/* Ok, I haven't implemented sigactions, but trying to keep headers POSIX */
// 子进程处于停止状态，就不对SIGchld处理
#define SA_NOCLDSTOP	1
// 不阻塞在指定信号处理程序（信号句柄）中再收到该信号
#define SA_NOMASK	0x40000000
// 信号句柄一旦被调用过就恢复到默认处理句柄
#define SA_ONESHOT	0x80000000

#define SIG_BLOCK          0	/* for blocking signals ， 在阻塞信号集上加上给定的信号集*/
#define SIG_UNBLOCK        1	/* for unblocking signals  从阻塞信号集中删除指定的信号集*/
#define SIG_SETMASK        2	/* for setting the signal mask 设置阻塞信号集， 信号屏蔽码*/

// 默认的信号处理函数
#define SIG_DFL		((void (*)(int))0)	/* default signal handling */
// 忽略信号处理函数
#define SIG_IGN		((void (*)(int))1)	/* ignore signal */

/**
 *
 */
struct sigaction {
	void (*sa_handler)(int);	// 信号处理函数
	sigset_t sa_mask;			// 对应信号的屏蔽码，在信号程序执行时， 阻塞这些信号的处理
	int sa_flags;				// 改变信号处理过程的信号集
	void (*sa_restorer)(void);	// 回复函数指针， 由libc提供，用于清理用户态堆栈。 
};

// 为信号_sigs何止一个新的信号处理函数
void (*signal(int _sig, void (*_func)(int)))(int);
// 向当前进程发送一个信号，等价于kill(geipid(), sig);
int raise(int sig);
// 向任何进程发出任何信号
int kill(pid_t pid, int sig);
// 向信号集中添加信号
int sigaddset(sigset_t *mask, int signo);
// 向信号集中删除指定的信号
int sigdelset(sigset_t *mask, int signo);
// 从信号集中清除指定信号集
int sigemptyset(sigset_t *mask);
// 向信号集中置入所有信号
int sigfillset(sigset_t *mask);
// 判断一个信号是不是信号集中的
int sigismember(sigset_t *mask, int signo); /* 1 - is, 0 - not, -1 error */
// 对set中信号进行检查， 看是否有挂起的信号
int sigpending(sigset_t *set);
// 改变目前的被阻塞的信号集。
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);
// 使用sigmask临时替换进程的信号屏蔽码，然后暂停该进程直到接收一个信号
int sigsuspend(sigset_t *sigmask);
// 改变进程在收到指定信号时所采取的行动
int sigaction(int sig, struct sigaction *act, struct sigaction *oldact);

#endif /* _SIGNAL_H */
