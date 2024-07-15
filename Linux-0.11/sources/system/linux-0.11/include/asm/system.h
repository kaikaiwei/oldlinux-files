// 定义了设置或修改描述符/中断门等的前入职汇编

/**
 * 内核在初始化结束时，切换到初始进程，任务0， 模拟中断调用的返回过程。
 */
#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \     // 保留堆栈指针，eax=esp
	"pushl $0x17\n\t" \					// 首相将task0堆栈段选择符ss入栈
	"pushl %%eax\n\t" \					// 然后将保存的额堆栈指针值入栈
	"pushfl\n\t" \						// 将标志寄存器eflags内容入栈
	"pushl $0x0f\n\t" \					// 将task0代码段选择符cs入栈
	"pushl $1f\n\t" \					// 将下面标号1的偏移地址入栈
	"iret\n" \							// 执行中断返回指令，则会跳转到下面标号1的
	"1:\tmovl $0x17,%%eax\n\t" \		// 此时开始执行任务0.  eax=0x17
	"movw %%ax,%%ds\n\t" \				// 初始化段寄存器执行本局部表的数据段
	"movw %%ax,%%es\n\t" \				// 
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")

// 开中断嵌入汇编宏函数
#define sti() __asm__ ("sti"::)
// 关中段
#define cli() __asm__ ("cli"::)
// 空操作
#define nop() __asm__ ("nop"::)

// 中断返回
#define iret() __asm__ ("iret"::)

// 设置门描述符宏函数
// gate_addr： 描述符嗲值
// type： 描述符中类型域值
// dlp： 描述符特权层值
// addr：偏移地址
#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \		// 将编译地址低字与段选择符组合成描述符低4字节
	"movw %0,%%dx\n\t" \			// 将类型标志与偏移高字组合成描述符高4字节
	"movl %%eax,%1\n\t" \			// 分别设置门描述符的低4字节和高4字节
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))

// 设置中断门，陷阱门和系统调用门宏函数，参数：n 中断号，addr 中断程序偏移地址
// &idt[n] 对应中断号在中断描述符表中的偏移值，中断描述符的类型时14，特权级是0
#define set_intr_gate(n,addr) \
	_set_gate(&idt[n],14,0,addr)

// idt对应的中段号在中断描述符表中的偏移值，中断描述符的类型时14，特权级是0
// 参数： n 中断号，
#define set_trap_gate(n,addr) \
	_set_gate(&idt[n],15,0,addr)

// idt对应的中段号在中断描述符表中的偏移值，中断描述符的类型时15，特权级是3
#define set_system_gate(n,addr) \
	_set_gate(&idt[n],15,3,addr)

// 设置段描述符函数
// 参数：gate_addr 描述符嗲值
// type 描述符中类型域值
// dpl 描述符特权层值
// base： 段的基地址
// limit： 段限长
#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\
	*(gate_addr) = ((base) & 0xff000000) | \
		(((base) & 0x00ff0000)>>16) | \
		((limit) & 0xf0000) | \
		((dpl)<<13) | \
		(0x00408000) | \
		((type)<<8); \
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \
		((limit) & 0x0ffff); }

// 载全局表中设置任务状态段， 局部表描述符
// n： 载全局表中描描述符项的下标
// addr： 状态段/局部表所在内存的基地址
// type： 描述符中的标志类型字节
#define _set_tssldt_desc(n,addr,type) \
__asm__ ("movw $104,%1\n\t" \		// 将tss长度放入描述符长度域， 0-1字节
	"movw %%ax,%2\n\t" \			// 将基地址的低字放入描述符第2-3字节
	"rorl $16,%%eax\n\t" \			// 将基地址高字移动到ax中
	"movb %%al,%3\n\t" \			// 将基地址高字中低字节一如描述符第4字节
	"movb $" type ",%4\n\t" \		// 将标志类型字节一如描述符的第5字节。
	"movb $0x00,%5\n\t" \			// 描述符的第6字节置0
	"movb %%ah,%6\n\t" \			// 将基地址高字中高字节一如描述符第7字节
	"rorl $16,%%eax" \				// eax恢复原值
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \
	)

// 在全局表中设置任务状态段描述符和局部表描述符
// n是该描述符的指针，addr是描述符中的基址。任务状态段描述符的类型时0x89
#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89")
// // n是该描述符的指针，addr是描述符中的基址。局部表描述符的类型时0x82
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82")
