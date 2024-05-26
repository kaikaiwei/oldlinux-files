/*
 *  linux/kernel/sys.c
 *  包括很多系统调用功能的实现函数。ENOSYS表示系统还没有实现这个功能。
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>

// 返回日期和时间
int sys_ftime()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

// 用于当前进程对子进程进行调试
int sys_ptrace()
{
	return -ENOSYS;
}

// 改变并打印终端行设置
int sys_stty()
{
	return -ENOSYS;
}

// 去终端行设置
int sys_gtty()
{
	return -ENOSYS;
}

// 修改文件名
int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

/** 
 * 设置当前任务的实际和有效用户组id
 * 如果当前用户没有超级用户权限。 
 */
int sys_setregid(int rgid, int egid)
{
	if (rgid>0) {
		if ((current->gid == rgid) || 
		    suser())
			current->gid = rgid;
		else
			return(-EPERM);
	}
	if (egid>0) {
		if ((current->gid == egid) ||
		    (current->egid == egid) ||
		    (current->sgid == egid) ||
		    suser())
			current->egid = egid; // 可以设置有效组id
		else
			return(-EPERM);
	}
	return 0;
}

// 设置进程组号
int sys_setgid(int gid)
{
	return(sys_setregid(gid, gid));
}

// 打开或关闭程序记账功能
int sys_acct()
{
	return -ENOSYS;
}

// 映射任务物理内存到进程的虚拟地址空间
int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

// 返回操作系统时间。 如果tloc不为null，则时间也存入tloc指向的位置。
int sys_time(long * tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc) {
		verify_area(tloc,4);
		put_fs_long(i,(unsigned long *)tloc);
	}
	return i;
}

/*
 * Unprivileged users may change the real user id to the effective uid
 * or vice versa.
 * 设置有效用户id
 */
int sys_setreuid(int ruid, int euid)
{
	int old_ruid = current->uid;
	
	if (ruid>0) {
		if ((current->euid==ruid) ||
                    (old_ruid == ruid) ||
		    suser())
			current->uid = ruid;
		else
			return(-EPERM);
	}
	if (euid>0) {
		if ((old_ruid == euid) ||
                    (current->euid == euid) ||
		    suser())
			current->euid = euid;
		else {
			current->uid = old_ruid;
			return(-EPERM);
		}
	}
	return 0;
}

int sys_setuid(int uid)
{
	return(sys_setreuid(uid, uid));
}

// 设置系统时间和日期
int sys_stime(long * tptr)
{
	if (!suser())	// 如果不是超级用户则返回
		return -EPERM;
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies/HZ;
	return 0;
}

// 获取当前任务时间。
// tms中包含用户时间，系统时间，子进程用户时间，子进程系统时间
int sys_times(struct tms * tbuf)
{
	if (tbuf) {
		verify_area(tbuf,sizeof *tbuf);
		put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);
		put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);
		put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime);
		put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime);
	}
	return jiffies;
}

/**
 * 设置数据段末尾为end_data_seg指定的值。该值必须大于代码结尾并且小于堆栈结尾16KB。
 * @param end_data_seg， 如果数值合理，切系统确实有足够的额内存，切进程没有超越其最大数据段大小时。
 */
int sys_brk(unsigned long end_data_seg)
{
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 * 将pid指定的进程的进程组id设置为pgid
 */
int sys_setpgid(int pid, int pgid)
{
	int i;

	// pid==0，就使用当前进程的pid
	if (!pid)
		pid = current->pid;
	
	// pgid==0， 就使用当前进程的pid
	if (!pgid)
		pgid = current->pid;

	// 遍历所有任务
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && task[i]->pid==pid) {
			// leader进程，则返回
			if (task[i]->leader)
				return -EPERM;
			
			// 如果该任务的会话id与当前进程的会话id不一样，则返回
			if (task[i]->session != current->session)
				return -EPERM;
			task[i]->pgrp = pgid; // 设置pgrp。 进程组id
			return 0;
		}
	return -ESRCH;
}

int sys_getpgrp(void)
{
	return current->pgrp;
}

// 创建一个会话。
int sys_setsid(void)
{
	if (current->leader && !suser())
		return -EPERM;
	current->leader = 1;	// leader进程
	current->session = current->pgrp = current->pid; // 进程组id为会话id
	current->tty = -1;		// 当前进程没有控制终端
	return current->pgrp;
}

/**
 * 获取系统信息。 utsname包含5个字段，分别是：本操作系统的名称，网络节点名称，当前发行级别，版本级别和硬件类型名称
 */ 
int sys_uname(struct utsname * name)
{
	static struct utsname thisname = {
		"linux .0","nodename","release ","version ","machine "
	};
	int i;

	if (!name) return -ERROR;
	verify_area(name,sizeof *name);
	for(i=0;i<sizeof *name;i++)
		// 将thisname中的数据逐字节拷贝到用户空间。
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);
	return 0;
}

/**
 * 设置当前进程 创建文件属性屏蔽码。
 */
int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}
