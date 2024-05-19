/*
 *  linux/kernel/exit.c
 *  进程终止和退出的描述。主要包含进程释放，会话终止和程序退出处理函数。
 *	以及杀死进程/终止进程/刮起进程等系统调用函数。
 *	信号发送函数send_sig和通知父进程子进程已经终止的函数tell_father
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <asm/segment.h>

int sys_pause(void);	// kernel/sched.c中
int sys_close(int fd);	// fs/open.c中

// 释放指定的进程。 释放任务槽及任务数据结构所占用的内存
void release(struct task_struct * p)
{
	int i;

	// 如果p为NULL，退出
	if (!p)
		return;
	// 扫描任务数组，寻找指定任务。
	for (i=1 ; i<NR_TASKS ; i++)
		if (task[i]==p) {	
			task[i]=NULL;		// 置NULL 该任务并释放相关内存页
			free_page((long)p);	// 释放内存页
			schedule();			// 重新调度
			return;				// 本程序返回
		}
	panic("trying to release non-existent task");
}

/**
 * 向指定任务p发送信号（sig），权限为priv
 * @param p 目标任务
 * @param sig 信号
 * @param priv 权限
 */
static inline int send_sig(long sig,struct task_struct * p,int priv)
{
	// 参数教研
	if (!p || sig<1 || sig>32)
		return -EINVAL;
	// 若有券或进程有效用户表示符是指定进程的euid或者是超级用户。则在进程位图中添加该信号。
	// 否则出错处理。
	if (priv || (current->euid==p->euid) || suser())
		p->signal |= (1<<(sig-1));
	else
		return -EPERM;
	return 0;
}

/**
 * 终止回话
 */
static void kill_session(void)
{
	struct task_struct **p = NR_TASKS + task;	// 指向任务数组最末端
	
	// 对所有的任务（任务0除外），如果其回话等于当前进程的回话，就像他发送挂断进程信号。
	while (--p > &FIRST_TASK) {
		if (*p && (*p)->session == current->session)
			(*p)->signal |= 1<<(SIGHUP-1);
	}
}

/*
 * XXX need to check permissions needed to send signals to process
 * groups, etc. etc.  kill() permissions semantics are tricky!
 * 向指定进程发送kill信号。
 * @param pid 进程号
 * @param sig  信号量。
 */
int sys_kill(int pid,int sig)
{
	struct task_struct **p = NR_TASKS + task; // 指向进程结束部分
	int err, retval = 0;

	// 如果pid=0， 会发给当前进程组中所有进程
	if (!pid) while (--p > &FIRST_TASK) {
		if (*p && (*p)->pgrp == current->pid) 
			if (err=send_sig(sig,*p,1))
				retval = err;
	} else if (pid>0) while (--p > &FIRST_TASK) { // 如果pid>0则信号会被发送给pid
		if (*p && (*p)->pid == pid) 
			if (err=send_sig(sig,*p,0))
				retval = err;
	} else if (pid == -1) while (--p > &FIRST_TASK)	// pid==-1， 会发送给除了第一个进程之外的所有进程
		if (err = send_sig(sig,*p,0))
			retval = err;
	else while (--p > &FIRST_TASK)	// pid < -1, 发送给-pid进程组的所有进程
		if (*p && (*p)->pgrp == -pid)
			if (err = send_sig(sig,*p,0))
				retval = err;
	return retval;
}

/**
 * 告知父进程子进程已经挂掉了
 */
static void tell_father(int pid)
{
	int i;

	if (pid)
		for (i=0;i<NR_TASKS;i++) {	// 遍历所有进程。
			if (!task[i])
				continue;
			if (task[i]->pid != pid)
				continue;
			task[i]->signal |= (1<<(SIGCHLD-1)); // 发送SIGCHLD信号。
			return;
		}
/* if we don't find any fathers, we just release ourselves */
/* This is not really OK. Must change it to make father 1 */
	printk("BAD BAD - no father found\n\r");
	release(current);
}

/**
 * 进程退出
 * @param code 退出码
 */
int do_exit(long code)
{
	int i;

	// 释放当前进程代码段和数据段所占有的内存页。 free_page_tables在mm/memory.c中
	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));
	// 遍历所有进程，如果当前进程有子进程，则子进程的father设置为1号进程。
	// 如果该子进程是僵尸进程，则向进程1发送子进程终止信号SIGCHLD
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && task[i]->father == current->pid) {
			task[i]->father = 1;
			if (task[i]->state == TASK_ZOMBIE)
				/* assumption task[1] is always init */
				(void) send_sig(SIGCHLD, task[1], 1);
		}
	
	// 遍历该进程打开的文件列表。关闭文件
	for (i=0 ; i<NR_OPEN ; i++)
		if (current->filp[i])
			sys_close(i);	
	// 对当前进程工作目录，根目录，以及运行程序的i节点进行同步操作，分别置空
	iput(current->pwd);
	current->pwd=NULL;
	iput(current->root);
	current->root=NULL;
	iput(current->executable);
	current->executable=NULL;

	// 如果当前进程是leader进程，并且有控制终端，则释放该终端
	if (current->leader && current->tty >= 0)
		tty_table[current->tty].pgrp = 0;
	
	// 如果该进程使用了协处理器，则协处理器置空
	if (last_task_used_math == current)
		last_task_used_math = NULL;

	// 如果是leader进程，则终止所有的相关进程
	if (current->leader)
		kill_session();

	// 当前进程标记为
	current->state = TASK_ZOMBIE;
	// 设置退出码
	current->exit_code = code;
	// 告知父进程
	tell_father(current->father);
	// 重新调度进程运行
	schedule();
	return (-1);	/* just to suppress warnings */
}

/**
 * 系统调用exit()， 终止当前进程
 * @param error_code 错误码
 */
int sys_exit(int error_code)
{
	return do_exit((error_code&0xff)<<8);
}

/**
 * 挂起当前进程，直到pid指定的子进程退出（终止）或者收到要求终止该进程的信号。
 * @param pid  指定的子进程。 >0表示等待指定进程。 等于0表示等待进程组号等于当前进程的任何子进程。 
 *             小于-1表示等待进程组号等于pid绝对值的任何子进程
 *             等于-1表示等待任何子进程
 * @param stat_addr 用于保存状态信息
 * @param options  WNOHANG，表示如果子进程是挺值的，马上返回。 WUNTRACED，表示如果没有子进程退出或终止就马上返回。include/wait.h中
 */
int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options)
{
	int flag, code;
	struct task_struct ** p;

	verify_area(stat_addr,4);
repeat:
	flag=0;
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p) {	// 从任务数组末端开始扫描所有任务
		// 跳过空任务和当前任务
		if (!*p || *p == current)
			continue;
		// 如果不是当前任务的子进程，跳过
		if ((*p)->father != current->pid)
			continue;
		
		if (pid>0) {	// 对于pid大于0， 跳过非pid的进程
			if ((*p)->pid != pid)
				continue;
		} else if (!pid) { // 如果pid等于0， 跳过非本进程组的
			if ((*p)->pgrp != current->pgrp)
				continue;
		} else if (pid != -1) {	// pid=-1， 跳过非-pid的
			if ((*p)->pgrp != -pid)
				continue;
		}

		// 对于进程状态进行判断
		switch ((*p)->state) {
			case TASK_STOPPED:		// 已经终止的
				if (!(options & WUNTRACED))
					continue;
				put_fs_long(0x7f,stat_addr);	// 置状态信息为0x7f
				return (*p)->pid;
			case TASK_ZOMBIE:
				// 更新内核态和用户态的时间
				current->cutime += (*p)->utime;
				current->cstime += (*p)->stime;
				flag = (*p)->pid;
				code = (*p)->exit_code;	// 取子进程的退出码
				release(*p);			// 释放该子进程
				put_fs_long(code,stat_addr);	// 置状态信息为退出码
				return flag;			// 退出，返回子程序的pid
			default:
				flag=1;			// 默认情况下flag为1
				continue;
		}
	}
	if (flag) {				// 处理默认情况。子进程没有处于退出或者僵死状态
		if (options & WNOHANG)	// 如果optins为WNOHANG，则立即返回
			return 0;
		current->state=TASK_INTERRUPTIBLE; // 任务设置为可中断
		schedule();
		if (!(current->signal &= ~(1<<(SIGCHLD-1)))) // 如果还是没收到SIGCHLD，重复
			goto repeat;
		else
			return -EINTR;
	}
	return -ECHILD;
}


