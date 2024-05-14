/*
 *  linux/kernel/traps.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'Traps.c' handles hardware traps and faults after we have saved some
 * state in 'asm.s'. Currently mostly a debugging-aid, will be extended
 * to mainly kill the offending process (probably by giving it a signal,
 * but possibly by killing it outright if necessary).
 * 主要包括一些在处理异常故障（硬件中断）的底层代码asm.s中调用的响应C函数。
 * 用于显示出错信息和出错号等调试信息。 die函数用于在中断处理中显示详细的出错信息。trap_init函数用于硬件异常处理中断向量的初始化，并设置允许中断信号的到来。
 */
#include <string.h>

#include <linux/head.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/io.h>

/** 
 * 嵌入式汇编语句。取段seg中地址addr处的一个字节。
 * 在栈中保存fs，将ax移动到fs中，将fs：addr转移到al中，fs出栈
 * 圆括号作为表达式使用
 */
#define get_seg_byte(seg,addr) ({ \
register char __res; \
__asm__("push %%fs;mov %%ax,%%fs;movb %%fs:%2,%%al;pop %%fs" \
	:"=a" (__res):"0" (seg),"m" (*(addr))); \
__res;})

/** 
 * 嵌入式汇编语句。取段seg中地址addr处的一个长字（4字节）
 * 在栈中保存fs，将ax移动到fs中，将fs：addr转移到al中，fs出栈
 * 圆括号作为表达式使用
 */
#define get_seg_long(seg,addr) ({ \
register unsigned long __res; \
__asm__("push %%fs;mov %%ax,%%fs;movl %%fs:%2,%%eax;pop %%fs" \
	:"=a" (__res):"0" (seg),"m" (*(addr))); \
__res;})

/** 
 * 嵌入式汇编语句。取fs寄存器的值。
 * 将fs保存在ax中，
 * 圆括号作为表达式使用
 */
#define _fs() ({ \
register unsigned short __res; \
__asm__("mov %%fs,%%ax":"=a" (__res):); \
__res;})

// 下面是一些函数原型。
int do_exit(long code);		// 程序推出操作。在kernel/exit.c


void page_exception(void);	// 页异常，世纪时page_fault， 命名，pages.s中

// 中断处理程序的原型，代码在kernel/asm.s或systemcall.s中。
void divide_error(void);	// int0， kernel/ams.s
void debug(void);			// int1， kernel/ams.s
void nmi(void);				// int2， kernel/ams.s
void int3(void);			// int3， kernel/ams.s
void overflow(void);		// int4， kernel/ams.s
void bounds(void);			// int5， kernel/ams.s
void invalid_op(void);		// int6， kernel/ams.s
void device_not_available(void);	// int7， kernel/ams.s
void double_fault(void);			// int8， kernel/ams.s
void coprocessor_segment_overrun(void);	// int9， kernel/ams.s
void invalid_TSS(void);				// int10， kernel/ams.s
void segment_not_present(void);		// int11， kernel/ams.s
void stack_segment(void);			// int12， kernel/ams.s
void general_protection(void);		// int13， kernel/ams.s
void page_fault(void);				// int14， mm/pages.s
void coprocessor_error(void);		// int16， kernel/ams.s
void reserved(void);				// int39， kernel/system_call.s
void parallel_interrupt(void);		// int45，协处理器中断处理 kernel/ams.s
void irq13(void);

/**
 * 该子程序用来打印出错中断的名称，出错号，调用程序的eip,eflags,esp,fs段寄存器值，
 * 段的基址，段的长度，进程号pid，任务号，10字节指令吗，如果堆栈在用户端，还打印16字节的堆栈内容
 * @param str
 * @param esp
 * @param nr
 */
static void die(char * str,long esp_ptr,long nr)
{
	long * esp = (long *) esp_ptr;
	int i;

	printk("%s: %04x\n\r",str,nr&0xffff); // 打印出错信息和nr（任务号）
	/// 打印eip， eflags，esp
	printk("EIP:\t%04x:%p\nEFLAGS:\t%p\nESP:\t%04x:%p\n",
		esp[1],esp[0],esp[2],esp[4],esp[3]);
	// 打印fs段寄存器内容
	printk("fs: %04x\n",_fs());
	// 打印base指针，ldtr。 和limit
	printk("base: %p, limit: %p\n",get_base(current->ldt[1]),get_limit(0x17));
	// 如果是用户堆栈。
	if (esp[4] == 0x17) {
		// 打印16字节的堆栈信息
		printk("Stack: ");
		for (i=0;i<4;i++)
			printk("%p ",get_seg_long(0x17,i+(long *)esp[3]));
		printk("\n");
	}
	str(i);	// 去当前运行任务的任务号。include/linux/sched.h
	// 打印pid和任务号
	printk("Pid: %d, process nr: %d\n\r",current->pid,0xffff & i);
	// 打印10字节的指令码
	for(i=0;i<10;i++)
		printk("%02x ",0xff & get_seg_byte(esp[1],(i+(char *)esp[0])));
	printk("\n\r");
	do_exit(11);		/* play segment exception */
}

void do_double_fault(long esp, long error_code)
{
	die("double fault",esp,error_code);
}

void do_general_protection(long esp, long error_code)
{
	die("general protection",esp,error_code);
}

void do_divide_error(long esp, long error_code)
{
	die("divide error",esp,error_code);
}

void do_int3(long * esp, long error_code,
		long fs,long es,long ds,
		long ebp,long esi,long edi,
		long edx,long ecx,long ebx,long eax)
{
	int tr;

	__asm__("str %%ax":"=a" (tr):"0" (0)); // 取任务寄存器
	printk("eax\t\tebx\t\tecx\t\tedx\n\r%8x\t%8x\t%8x\t%8x\n\r",
		eax,ebx,ecx,edx);
	printk("esi\t\tedi\t\tebp\t\tesp\n\r%8x\t%8x\t%8x\t%8x\n\r",
		esi,edi,ebp,(long) esp);
	printk("\n\rds\tes\tfs\ttr\n\r%4x\t%4x\t%4x\t%4x\n\r",
		ds,es,fs,tr);
	printk("EIP: %8x   CS: %4x  EFLAGS: %8x\n\r",esp[0],esp[1],esp[2]);
}

void do_nmi(long esp, long error_code)
{
	die("nmi",esp,error_code);
}

void do_debug(long esp, long error_code)
{
	die("debug",esp,error_code);
}

void do_overflow(long esp, long error_code)
{
	die("overflow",esp,error_code);
}

void do_bounds(long esp, long error_code)
{
	die("bounds",esp,error_code);
}

void do_invalid_op(long esp, long error_code)
{
	die("invalid operand",esp,error_code);
}

void do_device_not_available(long esp, long error_code)
{
	die("device not available",esp,error_code);
}

void do_coprocessor_segment_overrun(long esp, long error_code)
{
	die("coprocessor segment overrun",esp,error_code);
}

void do_invalid_TSS(long esp,long error_code)
{
	die("invalid TSS",esp,error_code);
}

void do_segment_not_present(long esp,long error_code)
{
	die("segment not present",esp,error_code);
}

void do_stack_segment(long esp,long error_code)
{
	die("stack segment",esp,error_code);
}

void do_coprocessor_error(long esp, long error_code)
{
	if (last_task_used_math != current)
		return;
	die("coprocessor error",esp,error_code);
}

void do_reserved(long esp, long error_code)
{
	die("reserved (15,17-47) error",esp,error_code);
}

/**
 * 陷阱门（中断向量）初始化
 * set_trap_gate和set_system_gate的区别在于前者设置的特权级位3，后者为0
 * 因此断点int3，溢出overflow，边界出错中断bounds可有任何程序产生。
 * 这两个函数是嵌入式汇编宏，在include/asm/system.h中。
 */
void trap_init(void)
{
	int i;

	set_trap_gate(0,&divide_error);		// 设置除中断的向量
	set_trap_gate(1,&debug);
	set_trap_gate(2,&nmi);
	set_system_gate(3,&int3);	/* int3-5 can be called from all */
	set_system_gate(4,&overflow);
	set_system_gate(5,&bounds);
	set_trap_gate(6,&invalid_op);
	set_trap_gate(7,&device_not_available);
	set_trap_gate(8,&double_fault);
	set_trap_gate(9,&coprocessor_segment_overrun);
	set_trap_gate(10,&invalid_TSS);
	set_trap_gate(11,&segment_not_present);
	set_trap_gate(12,&stack_segment);
	set_trap_gate(13,&general_protection);
	set_trap_gate(14,&page_fault);
	set_trap_gate(15,&reserved);
	set_trap_gate(16,&coprocessor_error);
	// 将17到48的陷阱门设置为reserved。在之后的硬件初始化时会重新设置自己的陷阱门。
	for (i=17;i<48;i++)
		set_trap_gate(i,&reserved);
	set_trap_gate(45,&irq13);		// 设置数字协处理器
	outb_p(inb_p(0x21)&0xfb,0x21);	// 允许主8259A芯片irq2中断
	outb(inb_p(0xA1)&0xdf,0xA1);	// 允许主8259A芯片irq3中断
	set_trap_gate(39,&parallel_interrupt);	// 设置并行口陷阱门。
}
