/*
 *  linux/kernel/signal.c
 *	设置和获取进程信号阻塞码（屏蔽码）系统调用函数sys_ssetmask和syssgetmask， 
 * 	信号处理函数sys_signal， 修改进程在收到特性信号时所采用的行动调用sys_sigaction(), 以及在系统调用
 *	中断处理程序中处理信号的函数do_signal（）。
 * 	信号发送函数send_sig和通知符进程函数tell_father（）在exec.c中。
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <signal.h>

// volatile 要求编译器不要对齐进行优化。
volatile void do_exit(int error_code);

// 获取当前进程的信号屏蔽位图
int sys_sgetmask()
{
	return current->blocked;
}

// 设置当前进程的信号屏蔽位图。
int sys_ssetmask(int newmask)
{
	int old=current->blocked;

	current->blocked = newmask & ~(1<<(SIGKILL-1)); // 除了SIGNAL_KILL 外的均可有用户指定
	return old;
}

/**
 * 内联函数。 保存旧的sigaction
 * @param from 原sigaction地址
 * @param to 新sigaction地址， fs段to处
 */
static inline void save_old(char * from,char * to)
{
	int i;

	verify_area(to, sizeof(struct sigaction)); // 内存区域验证，
	// 进行字节拷贝
	for (i=0 ; i< sizeof(struct sigaction) ; i++) {
		put_fs_byte(*from,to);	// 放到fs寄存器的to（偏移）处
		from++;
		to++;
	}
}

/**
 * 获取新的sigaction。
 * @param from 来源 fs的from偏移处
 * @param to 目标地址
 */
static inline void get_new(char * from,char * to)
{
	int i;

	for (i=0 ; i< sizeof(struct sigaction) ; i++)
		*(to++) = get_fs_byte(from++);
}

/**
 * signal系统调用，为指定信号安装新的信号句柄。
 * @param signum 信号量 
 * @param handler 信号句柄，可以是自定义的函数，也可以是SIG_DFL(默认句柄)
 * @param restorer 恢复函数指针，该还有有libc库提供
 */
int sys_signal(int signum, long handler, long restorer)
{
	struct sigaction tmp;

	// 信号值要在1-32范围内，并且不能是SIGKILL
	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;
	
	// 为sigaciton进行初始化操作。
	tmp.sa_handler = (void (*)(int)) handler;
	tmp.sa_mask = 0;
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;			// 执行信号的屏蔽码，这里只使用一次。一次后就会回复原来的。
	tmp.sa_restorer = (void (*)(void)) restorer;	// 回复函数指针
	handler = (long) current->sigaction[signum-1].sa_handler; // 原来的处理句柄
	current->sigaction[signum-1] = tmp;		// 设置为新的处理句柄
	return handler;
}

/**
 * sigaction系统调用，为指定信号安装新的信号句柄。
 * @param signum 信号量 
 * @param handler 信号句柄，可以是自定义的函数，也可以是SIG_DFL(默认句柄)
 * @param oldaction 用于接收旧的sigaction
 */
int sys_sigaction(int signum, const struct sigaction * action,
	struct sigaction * oldaction)
{
	struct sigaction tmp;

	// 信号值要在1-32范围内，并且不能是SIGKILL
	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;
	// 得到原来的句柄
	tmp = current->sigaction[signum-1];
	get_new((char *) action,
		(char *) (signum-1+current->sigaction));
	
	// 如果有原来的句柄。则保存到oldaction中
	if (oldaction)
		save_old((char *) &tmp,(char *) oldaction);
	
	// 设置sa_mask
	if (current->sigaction[signum-1].sa_flags & SA_NOMASK)
		current->sigaction[signum-1].sa_mask = 0;
	else
		current->sigaction[signum-1].sa_mask |= (1<<(signum-1));
	return 0;
}

/**
 * 在中断处理程序中被调用。见system_call.c
 * 主要是将信号的处理句柄插入到用户程序堆栈中，并在本系统调用结束返回后立即执行信号句柄程序，然后继续执行用户的程序。
 * @param signr 表示信号量
 */
void do_signal(long signr,long eax, long ebx, long ecx, long edx,
	long fs, long es, long ds,
	long eip, long cs, long eflags,
	unsigned long * esp, long ss)
{
	unsigned long sa_handler;
	long old_eip=eip;
	struct sigaction * sa = current->sigaction + signr - 1; // 得到信号处理句柄
	int longs;
	unsigned long * tmp_esp;

	sa_handler = (unsigned long) sa->sa_handler;
	if (sa_handler==1)		// 1表示SIG_IGN, 忽略的意思
		return;
	if (!sa_handler) {			// 如果没有指定，默认处理
		if (signr==SIGCHLD)		// 父进程发出，终止或停止子进程。
			return;
		else
			do_exit(1<<(signr-1));	// 否则，退出程序
	}
	
	// 如果该句柄只使用一次
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;

	// 用信号句柄替换内核堆栈中源用户程序eip，同时也将之前的 eax，esp等入栈，方便从sigaction处理函数返回。
	*(&eip) = sa_handler;					// 程序指针
	longs = (sa->sa_flags & SA_NOMASK)?7:8; // 如果信号的处理句柄接收到自己
	*(&esp) -= longs;						// 堆栈指针
	verify_area(esp,longs*4);				// 内存验证
	tmp_esp=esp;							// 保存esp
	put_fs_long((long) sa->sa_restorer,tmp_esp++);	// 保存寄存器值到fs中。
	put_fs_long(signr,tmp_esp++);
	if (!(sa->sa_flags & SA_NOMASK))
		put_fs_long(current->blocked,tmp_esp++);
	put_fs_long(eax,tmp_esp++);
	put_fs_long(ecx,tmp_esp++);
	put_fs_long(edx,tmp_esp++);
	put_fs_long(eflags,tmp_esp++);
	put_fs_long(old_eip,tmp_esp++);
	current->blocked |= sa->sa_mask;		// 更新屏蔽码位图， 添加上sa_mask
}
