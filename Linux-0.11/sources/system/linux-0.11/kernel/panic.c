/*
 *  linux/kernel/panic.c
 *  《银河徒步旅行者指南》
 *  (C) 1991  Linus Torvalds
 */

/*
 * This function is used through-out the kernel (includeinh mm and fs)
 * to indicate a major problem.
 */
#include <linux/kernel.h>
#include <linux/sched.h>

void sys_sync(void);	/* it's really int */ // fs_buffers.c中，用于同步数据到磁盘中。

volatile void panic(const char * s)
{
	printk("Kernel panic: %s\n\r",s); // 打印输出字符
	if (current == task[0])		// 如果来自任务0， 则
		printk("In swapper task - not syncing\n\r");
	else
		sys_sync();
	for(;;);	// 死循环，死机
}
