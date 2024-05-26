/*
 *  linux/kernel/printk.c
 *  内核使用的显示函数。 使用本文件的原因是：载内核中不能使用专用于用户模式的fs段寄存器。需要首先保存fs寄存器的值。
 *  首先使用vsprintf对参数进行格式化处理。 然后在保存了fs段寄存器的情况下调用tty_write进行信息的打印显示。
 *  (C) 1991  Linus Torvalds
 */

/*
 * When in kernel-mode, we cannot use printf, as fs is liable to
 * point to 'interesting' things. Make a printf with fs-saving, and
 * all is well.
 */
#include <stdarg.h>
#include <stddef.h>

#include <linux/kernel.h>

static char buf[1024];

extern int vsprintf(char * buf, const char * fmt, va_list args);

int printk(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);
	__asm__("push %%fs\n\t"	// 保存fs
		"push %%ds\n\t"
		"pop %%fs\n\t"		// 令fs=ds。 将栈顶的值写入fs中。
		"pushl %0\n\t"		// 将字符串长度压入堆栈
		"pushl $_buf\n\t"	// 将buf地址压入堆栈
		"pushl $0\n\t"		// 将数值0压入堆栈。这里表示通道号
		"call _tty_write\n\t"	// 调用tty_write函数
		"addl $8,%%esp\n\t"		// 栈会退8字节
		"popl %0\n\t"			// 弹出字符串长度值，作为返回值
		"pop %%fs"				// 回复原来fs寄存器的值
		::"r" (i):"ax","cx","dx");
	return i;
}
