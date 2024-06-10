/*
 * linux/kernel/math/math_emulate.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This directory should contain the math-emulation code.
 * Currently only results in a signal.
 * 数学方针信号，目前只包含一个代码
 */

#include <signal.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

/**
 * 协处理器方针函数。中断处理程序调用的c函数，kernel/math/system_call.s
 */
void math_emulate(long edi, long esi, long ebp, long sys_call_ret,
	long eax,long ebx,long ecx,long edx,
	unsigned short fs,unsigned short es,unsigned short ds,
	unsigned long eip,unsigned short cs,unsigned long eflags,
	unsigned short ss, unsigned long esp)
{
	unsigned char first, second; 

	//0x0007 表示用户空间的代码
/* 0x0007 means user code space */
	// 0x000F，表示在局部描述符表中描述符索引值=1，即代码空间。
	// 如果段寄存器cs不等于0x000F， 则表示cs一定是内核代码选择符，也就是在内核代码空间。出错处理
	if (cs != 0x000F) {
		printk("math_emulate: %04x:%08x\n\r",cs,eip);
		panic("Math emulation needed in kernel");
	}
	// 取进程cs：eip执行的2字节代码first和second。 显示这些数据。并给进程设置浮点异常信号SIGFPE
	first = get_fs_byte((char *)((*&eip)++));
	second = get_fs_byte((char *)((*&eip)++));
	printk("%04x:%08x %02x %02x\n\r",cs,eip-2,first,second);
	current->signal |= 1<<(SIGFPE-1);
}

/**
 *
 */
void math_error(void)
{
	__asm__("fnclex");
	if (last_task_used_math)
		last_task_used_math->signal |= 1<<(SIGFPE-1);
}
