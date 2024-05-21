/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 * fork函数的辅助函数
 */
#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

// 写区域验证，mm/memory.c中
extern void write_verify(unsigned long address);

long last_pid=0; // 上一个pid

/**
 * 写操作前验证函数。
 * @param addr， 写入地址
 * @param size， 写入长度
 */
void verify_area(void * addr,int size)
{
	unsigned long start;

	start = (unsigned long) addr;
	size += start & 0xfff; // 4kB得结束地址
	start &= 0xfffff000;	// start 为开始地址
	start += get_base(current->ldt[2]);	// 得到基地址。系统线性空间中的地址
	while (size>0) {
		size -= 4096;	// 减去4kB
		write_verify(start); // 写页面验证，如果不可写，则进行复制页面
		start += 4096;
	}
}

/**
 * 设置新任务的代码和数据段基址，限长并复制页表。nr为新任务号；p是新任务数据结构指针
 * @param nr 任务号
 * @param p 新任务数据结构指针
 */
int copy_mem(int nr,struct task_struct * p)
{
	unsigned long old_data_base,new_data_base,data_limit;
	unsigned long old_code_base,new_code_base,code_limit;

	code_limit=get_limit(0x0f);	// 取局部描述符表中代码段描述符项中段限长
	data_limit=get_limit(0x17);	// 去局部描述符表中数据段描述符项段限长
	old_code_base = get_base(current->ldt[1]);	// 取原代码段基址
	old_data_base = get_base(current->ldt[2]);	// 取原数据段基址
	// 0.11不支持代码和数据段分立的情况
	if (old_data_base != old_code_base)			
		panic("We don't support separate I&D");
	// 数据段长度小于代码段长度，也是一种异常情况
	if (data_limit < code_limit)
		panic("Bad data_limit");
	// 新的代码段和数据段起始地址为 任务号*64MB
	new_data_base = new_code_base = nr * 0x4000000;
	p->start_code = new_code_base;
	set_base(p->ldt[1],new_code_base);	// 设置代码段描述符中基址域
	set_base(p->ldt[2],new_data_base);	// 设置数据段描述符中基址域
	
	// 拷贝页面操作
	// 设置新进程的页目录表项和页表项。也就是把新进程的线性地址内存页对应到实际物理地址内存页面上。
	// linux采用写时复制，这里仅为新进程设置了自己的页目录表项和页表项。没有实际为新进程分配物理页面。
	// 此时新进程与父进程共享所有的物理页面
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {
		free_page_tables(new_data_base,data_limit); // 如果出错，则释放页描项
		return -ENOMEM;
	}
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 * 复制进程。
 * @param nr 任务号
 * @param ebp
 * @param edi
 * @param esi
 * @param gs
 * @param none system.call.s中调用sys_call_table时压入堆栈的返回地址
 * @param ebx
 * @param ecx
 * @param edx
 * @param fs
 * @param es
 * @param ds
 * @param eip
 * @param cs
 * @param ds
 * @param eip
 * @param cs
 * @param eflsgs
 * @param esp
 * @param ss
 */
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	struct task_struct *p;
	int i;
	struct file *f;

	// 位新任务数据结构分配内存
	p = (struct task_struct *) get_free_page();
	if (!p)
		return -EAGAIN;

	// 将新任务指针放入数组中。nr时任务号
	task[nr] = p;
	// 这样做不会复制号机用户的堆栈，只复制当前进程的内容
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
	p->state = TASK_UNINTERRUPTIBLE;	// 设置状态位不可中段的异常
	p->pid = last_pid;					// 新进程号。 有之前调用find_empty_process得到。
	p->father = current->pid;			// 设置符进程号
	p->counter = p->priority;			// 时间片
	p->signal = 0;						// 信号位图
	p->alarm = 0;						// 时钟中断
	p->leader = 0;		/* process leadership doesn't inherit */ // 进程领导权不能继承
	p->utime = p->stime = 0;			// 用户态时间
	p->cutime = p->cstime = 0;			// 内核态时间
	p->start_time = jiffies;			// 开始运行时间。当前滴答数
	p->tss.back_link = 0;				// 设置当前tss段需要的数据。Task State Segment（任务状态段）
	p->tss.esp0 = PAGE_SIZE + (long) p;	// 堆栈指针
	p->tss.ss0 = 0x10;					// 堆栈段选择符（内核数据段）
	p->tss.eip = eip;					// 指令代码指针
	p->tss.eflags = eflags;				// 标志寄存器
	p->tss.eax = 0;						// 这是当fork返回时，新进程返回0的原因
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;					// 新进程完全复制了符进程的堆栈内容。 因此要求任务0的堆栈比较干净
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;			// 段寄存器仅16位有效
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);				// 该任务nr的局部描述符表选择符。（LDT的描述符在GDT中）
	p->tss.trace_bitmap = 0x80000000;	// 高16位有效

	// 如果当前任务使用了数学协处理器，就保存状态
	if (last_task_used_math == current)			
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));

	// 设置新任务的代码段和数据段的基址，限长并复制页表。 如果出错，则复位任务数组中对应项，并释放内存。
	if (copy_mem(nr,p)) {
		task[nr] = NULL;
		free_page((long) p);
		return -EAGAIN;
	}

	// 如果父进程中有文件时打开的，就增加文件的引用计数
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	// 将当前任务的pid，root和executable引用次数加1
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;

	// 在GDT中设置新任务的TSS和LDT描述符项。参见include/asm/system.h
	// 在任务切换时，任务寄存器tr有cpu自动加载。
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));
	p->state = TASK_RUNNING;	/* do this last, just in case */
	return last_pid;
}

/**
 * 查找空进程。为新进程获取不重复的进程号。 凭返回在任务数组中的任务号（下标）
 */
int find_empty_process(void)
{
	int i;

	// 查找可用的last_pid
	repeat:
		if ((++last_pid)<0) last_pid=1;
		for(i=0 ; i<NR_TASKS ; i++)
			if (task[i] && task[i]->pid == last_pid) goto repeat;

	// 排除任务0。 查找任务数组中空的项
	for(i=1 ; i<NR_TASKS ; i++)	
		if (!task[i])
			return i;
	return -EAGAIN;
}
