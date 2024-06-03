/*
 *  linux/kernel/keyboard.S
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	Thanks to Alfred Leung for US keyboard patches
 *		Wolfgang Thiel for German keyboard patches
 *		Marc Corsini for the French keyboard
 *  在英文的惯用法中，make表示按下，break表示被松开。
 *  对于AT键盘的扫描码， 当键按下式，对应键的扫描码被送出。 当键松开时，发送两个字节，第一个是0xf0，第二个还是按下式的扫描码。
 *  为了向下兼容， 设计人员将AT键盘发出的扫描码换成了老式PC/XT标准键盘的扫描码。这里只对XT的扫描码进行处理
 */

#include <linux/config.h>	// 内核配置头文件，定义键盘语言和硬盘类型（HD_TYPE)可选项

.text
.globl _keyboard_interrupt

/*
 * these are for the keyboard read functions
 * 以下用于键盘读操作。
 */
/* 键盘缓冲区长度，字节数 */
size	= 1024		/* must be a power of two ! And MUST be the same
			   as in tty_io.c !!!! */
head = 4		# 缓冲区中头指针字段偏移
tail = 8		# 缓冲区中尾指针字段偏移
proc_list = 12	# 等待该缓冲队列的进程字段偏移
buf = 16		# 缓冲区字段偏移

/*
 * mode 时键盘特殊键的按下状态标记
 * 表示大小写转换键 caps， 交换键alt， 控制键ctrl，换挡键 shift，的状态
 * 位7 表示caps键按下
 * 位6 表示caps键的状态
 * 位5 右alt键按下。
 * 位4 左alt键按下。
 * 位3 右ctrl键按下
 * 位2 左ctrl键按下
 * 位1 右shift键按下
 * 位0 左shift键按下
 */
mode:	.byte 0		/* caps, alt, ctrl and shift mode */
/*
 * 数字锁定键num-lock， 大小写转换键 caps， 滚动锁定键 scroll-lock的LED发光管状态
 * 位7-3 全0不用。 
 * 位2 caps-lock
 * 位1 num-lock，初始置1，表示发光
 * 位0 scroll-lock 
 */
leds:	.byte 2		/* num-lock, caps, scroll-lock mode (nom-lock on) */
/*
 * 当扫描码是0xe0或0xe1时，置该标志。 表示其后还跟随着1个或两个字符扫描码。见列表后说明。
 * 位1 =1，表示收到0x31。 位0=1，收到0xe0标志。
 */
e0:	.byte 0

/*
 *  con_int is the real interrupt routine that reads the
 *  keyboard scan-code and converts it into the appropriate
 *  ascii character(s).
 *  键盘中断处理子程序。用于读键盘扫描码并将其转换为相应的ascii字符。
 */
_keyboard_interrupt:
	pushl %eax		# 保存寄存器的值
	pushl %ebx
	pushl %ecx
	pushl %edx
	push %ds		# 保存ds，es段寄存器的值。为内核数据段。
	push %es
	movl $0x10,%eax		# 使用内核数据段。
	mov %ax,%ds
	mov %ax,%es
	xorl %al,%al		/* %eax is scan code  al为0 */
	inb $0x60,%al		# 读取扫描码al。
	cmpb $0xe0,%al		# 比较al中和0xe0， 
	je set_e0			# 两个相等，调用set_e0处
	cmpb $0xe1,%al		# 比较0xe1和al
	je set_e1			# 两个相等，跳转到set_e1处
	call key_table(,%eax,4)		# 调用键处理程序key_table + eax * 4. 
	movb $0,e0			# 复位e0表示。 e0=0
e0_e1:	inb $0x61,%al	# 读取PPI端口B转台，其位7用于允许/禁止（0/1）键盘
	jmp 1f				# 空指令
1:	jmp 1f
1:	orb $0x80,%al		# al位7置位， 禁止键盘工作
	jmp 1f
1:	jmp 1f
1:	outb %al,$0x61		# 使PPI 位7置位
	jmp 1f
1:	jmp 1f
1:	andb $0x7F,%al		# al位7复位
	outb %al,$0x61		# PPI PB7复位，允许键盘工作
	movb $0x20,%al		# 向8025A发送EOI（中断结束）信号。
	outb %al,$0x20		
	pushl $0			# 控制台tty号， = 0， 作为参数入栈
	call _do_tty_interrupt	# 将收到的数据复制成规范数据并存放在规范字符缓冲队列中
	addl $4,%esp		# 丢弃入栈参数，弹出保留寄存器，并从中断返回。
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret
set_e0:	movb $1,e0		# 收到扫描前导码0xe0时， 设置e0标志。
	jmp e0_e1
set_e1:	movb $2,e0		# 收到扫描前导码0xe1时， 设置e1标志。
	jmp e0_e1

/*
 * This routine fills the buffer with max 8 bytes, taken from
 * %ebx:%eax. (%edx is high). The bytes are written in the
 * order %al,%ah,%eal,%eah,%bl,%bh ... until %eax is zero.
 * 把edx， eax中最多8个字符添加缓冲队列中。 edx时所写入字符的顺序是al，ah，eal，eah。。。知道eax为0
 */
put_queue:
	pushl %ecx					# 保存ecx，edx中内容
	pushl %edx					
	movl _table_list,%edx		# read-queue for console 
	movl head(%edx),%ecx		# 取缓冲中队列头指针，ecx。
1:	movb %al,buf(%edx,%ecx)		# 将al中的字符放入缓冲队列头指针位置处
	incl %ecx					# 增加ecx。 向前移动指针
	andl $size-1,%ecx			# 以缓冲区大小调整头指针（若超出则返回缓冲区开始指针）
	cmpl tail(%edx),%ecx		# buffer full - discard everything 头指针==尾指针？，队列满了
	je 3f						# 如果已满，则后面为放入的字符全抛弃
	shrdl $8,%ebx,%eax			# 将ebx中8位比特位右移8位到eax中，但edx不变
	je 2f						# 还有字符吗？
	shrl $8,%ebx				# 将ebx中比特位右移8位，并向后跳转到1号位置。
	jmp 1b
2:	movl %ecx,head(%edx)		# 如果已经将所有字符都放入了队列，则保存头指针
	movl proc_list(%edx),%ecx	# 该队列的等待进程指针
	testl %ecx,%ecx				# 检测任务结构指针是否位空（右等待该队列的进程吗？）
	je 3f						# 无，则跳转
	movl $0,(%ecx)				# 右，则置该进程位可运行就绪状态。唤醒该进程
3:	popl %edx					# 弹出保留的寄存器并返回。
	popl %ecx
	ret

/**
 * 根据ctrl或alt的扫描码，分别设置模式标志中的相应位。如果该扫描码收到过0xe0扫描码（e0标志置位）， 则
 * 说明按下的是键盘右边的ctrl或alt键。则对应设置ctrl或alt在模式标志mode中的比特位
 */
ctrl:	movb $0x04,%al	# 0x04是左ctrl在mode中的比特位
	jmp 1f
alt:	movb $0x10,%al	# 0x10是左alt子啊mode中的比特位
1:	cmpb $0,e0			# e0 标志置位了吗？
	je 2f				# 没有置位，跳转
	addb %al,%al		# 置位了，改成相应的右键标志位。位3或位5.
2:	orb %al,mode		# 设置模式标志mode中对应的比特位
	ret

/**
 * 处理ctrl和alt松开的。
 */
unctrl:	movb $0x04,%al	# 左侧ctrl松开
	jmp 1f
unalt:	movb $0x10,%al	# 左侧alt松开
1:	cmpb $0,e0			# e0 标志置位了吗？
	je 2f				# 没有置位
	addb %al,%al		# 转换为对应的右键标志位。
2:	notb %al			# 没有置位。 取消al
	andb %al,mode		# 取消模式标志mode中对应的比特位
	ret

# shift 左侧。
lshift:
	orb $0x01,mode
	ret
unlshift:
	andb $0xfe,mode
	ret

# shift 右侧
rshift:
	orb $0x02,mode
	ret
unrshift:
	andb $0xfd,mode
	ret

# 大小写转换
caps:	testb $0x80,mode	# mode中位7是否已经置位
	jne 1f					# 如果已经处于按下状态。 则返回。234行
	xorb $4,leds			# 反转leds标志中caps-lock比特位。 位2
	xorb $0x40,mode			# 反转mode标志中cap键按下比特位。位6
	orb $0x80,mode			# 设置mode标志中caps键已经按下标位。位7，

# 根据leds标志， 开启或关闭led的命令。
set_leds:					
	call kb_wait		# 等待键盘控制器输入缓冲空
	movb $0xed,%al		/* set leds command。 设置LED命令 */
	outb %al,$0x60		# 发送键盘命令0xed到0x60端口
	call kb_wait		# 等待键盘控制器输入缓冲空
	movb leds,%al		# 取leds标志，作为参数
	outb %al,$0x60		# 发送该参数
	ret

# 关闭led命令
uncaps:	andb $0x7f,mode	# 将模式中对应的lead位清空，位7
	ret

# scroll 标志
scroll:
	xorb $1,leds	# scroll键按下，反转leds标志中的对应位。位0
	jmp set_leds	# 根据leds标志重新开启或关闭LED指示器
num:	xorb $2,leds	# num键按下， 反转leds中对应的标志位。位1
	jmp set_leds		# 根据leds标志重新开启或关闭LED指示器

/*
 *  curosr-key/numeric keypad cursor keys are handled here.
 *  checking for numeric keypad etc.  处理方向键，数字小键盘方向键，检测数字小键盘等
 */
cursor:
	subb $0x47,%al		# 扫描码是小数字键盘上的键。其扫描码>=0x47.
	jb 1f				# 如果小于则不处理
	cmpb $12,%al		# 如果扫描码大于0x53. 0x53-0x47=12.
	ja 1f				# 扫描码超过0x53（83），则不处理
	jne cur2		/* check for ctrl-alt-del。 检查是否ctrl-alt-del按下*/
	testb $0x0c,mode	# 如果mode为0x0c。 ctrl键按下
	je cur2				# 无，跳转到cur2
	testb $0x30,mode	# 如果mode为0x30。 alt键按下
	jne reboot			# 重新启动。见本文最后
cur2:	cmpb $0x01,e0		/* e0 forces cursor movement。 e0置位表示光标移动 */
	je cur				# 置位了，跳转光标移动处cur。
	testb $0x02,leds	/* not num-lock forces cursor。 num-lock键是否置位 */
	je cur				# 没有置位，进行光标移动处理
	testb $0x03,mode	/* shift forces cursor。 shift也进行光标移动处理 */
	jne cur				# shift键按下，也进行光标移动处理
	xorl %ebx,%ebx		# 否则查询数字表，取对应键的数字ascii码
	movb num_table(%eax),%al	# 以eax为索引值，取对应数字字符al。
	jmp put_queue			# 将该字符放入缓冲队列中
1:	ret

# 处理光标移动
cur:	movb cur_table(%eax),%al	# 取光标字符标中相应键的代表字符，al
	cmpb $'9,%al				# 若该字符<=9, 说明上一页，下一页，插入或删除键
	ja ok_cur					# 则功能字符序列中要添加字符～
	movb $'~,%ah				
ok_cur:	shll $16,%eax			# 将ax中的内容移动到eax中
	movw $0x5b1b,%ax			# ax中放入 esc[ 字符， 与eax高子中字符组成移动序列
	xorl %ebx,%ebx				# 
	jmp put_queue				# 将该字符放入到缓冲队列中

#if defined(KBD_FR)
num_table:
	.ascii "789 456 1230."		# 数字小键盘上键对应的数字ascii码表
#else
num_table:
	.ascii "789 456 1230,"		# 数字小键盘上键对应的数字ascii码表
#endif
cur_table:
	.ascii "HA5 DGC YB623"		# 数字小键盘上方向键或插入删除键对应的移动表示字符标

/*
 * this routine handles function keys。 处理功能键
 */
func:
	pushl %eax			# 寄存器保存
	pushl %ecx
	pushl %edx
	call _show_stat		# 调用显示个任务状态函数。 kernel/sched.c
	popl %edx
	popl %ecx
	popl %eax			# 寄存器出栈结束
	subb $0x3B,%al		# 功能键F1时0x3B。 al中时功能键的索引号
	jb end_func			# 扫描码小于0x3B。 则不处理
	cmpb $9,%al			# 功能键时F0-F10
	jbe ok_func			# 是，则跳转到处理函数
	subb $18,%al		# 不是。功能键时F11， F12吗
	cmpb $10,%al		# 时功能键F11吗
	jb end_func			# 不是，则不处理，返回
	cmpb $11,%al		# 功能键时F12吗？
	ja end_func			# 不是，则不处理
ok_func:
	cmpl $4,%ecx		/* check that there is enough room。 需要放入4个字符序列，放不下，则返回 */
	jl end_func				# 取功能键对应字符序列
	movl func_table(,%eax,4),%eax	# 取功能键对应字符序列
	xorl %ebx,%ebx
	jmp put_queue			# 放入到缓冲队列中
end_func:
	ret

/*
 * function keys send F1:'esc [ [ A' F2:'esc [ [ B' etc.
 * 功能键发送的扫描码， 
 */
func_table:
	.long 0x415b5b1b,0x425b5b1b,0x435b5b1b,0x445b5b1b
	.long 0x455b5b1b,0x465b5b1b,0x475b5b1b,0x485b5b1b
	.long 0x495b5b1b,0x4a5b5b1b,0x4b5b5b1b,0x4c5b5b1b

#if	defined(KBD_FINNISH)	# 芬兰语键盘的额扫描码映射表
key_map:
	.byte 0,27
	.ascii "1234567890+'"
	.byte 127,9
	.ascii "qwertyuiop}"
	.byte 0,13,0
	.ascii "asdfghjkl|{"
	.byte 0,0
	.ascii "'zxcvbnm,.-"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '<
	.fill 10,1,0

shift_map:					# shift键同时按下映射表
	.byte 0,27
	.ascii "!\"#$%&/()=?`"
	.byte 127,9
	.ascii "QWERTYUIOP]^"
	.byte 13,0
	.ascii "ASDFGHJKL\\["
	.byte 0,0
	.ascii "*ZXCVBNM;:_"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '>
	.fill 10,1,0

alt_map:					# alt键同时按下映射表
	.byte 0,0
	.ascii "\0@\0$\0\0{[]}\\\0"
	.byte 0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte '~,13,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte 0,0,0,0,0		/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '|
	.fill 10,1,0

#elif defined(KBD_US)		# 美式键盘

key_map:
	.byte 0,27
	.ascii "1234567890-="
	.byte 127,9
	.ascii "qwertyuiop[]"
	.byte 13,0
	.ascii "asdfghjkl;'"
	.byte '`,0
	.ascii "\\zxcvbnm,./"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '<
	.fill 10,1,0


shift_map:
	.byte 0,27
	.ascii "!@#$%^&*()_+"
	.byte 127,9
	.ascii "QWERTYUIOP{}"
	.byte 13,0
	.ascii "ASDFGHJKL:\""
	.byte '~,0
	.ascii "|ZXCVBNM<>?"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '>
	.fill 10,1,0

alt_map:
	.byte 0,0
	.ascii "\0@\0$\0\0{[]}\\\0"
	.byte 0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte '~,13,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte 0,0,0,0,0		/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '|
	.fill 10,1,0

#elif defined(KBD_GR)		# 德式键盘

key_map:
	.byte 0,27
	.ascii "1234567890\\'"
	.byte 127,9
	.ascii "qwertzuiop@+"
	.byte 13,0
	.ascii "asdfghjkl[]^"
	.byte 0,'#
	.ascii "yxcvbnm,.-"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '<
	.fill 10,1,0


shift_map:
	.byte 0,27
	.ascii "!\"#$%&/()=?`"
	.byte 127,9
	.ascii "QWERTZUIOP\\*"
	.byte 13,0
	.ascii "ASDFGHJKL{}~"
	.byte 0,''
	.ascii "YXCVBNM;:_"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '>
	.fill 10,1,0

alt_map:
	.byte 0,0
	.ascii "\0@\0$\0\0{[]}\\\0"
	.byte 0,0
	.byte '@,0,0,0,0,0,0,0,0,0,0
	.byte '~,13,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte 0,0,0,0,0		/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '|
	.fill 10,1,0


#elif defined(KBD_FR)		# 法式键盘

key_map:
	.byte 0,27
	.ascii "&{\"'(-}_/@)="
	.byte 127,9
	.ascii "azertyuiop^$"
	.byte 13,0
	.ascii "qsdfghjklm|"
	.byte '`,0,42		/* coin sup gauche, don't know, [*|mu] */
	.ascii "wxcvbn,;:!"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '<
	.fill 10,1,0

shift_map:
	.byte 0,27
	.ascii "1234567890]+"
	.byte 127,9
	.ascii "AZERTYUIOP<>"
	.byte 13,0
	.ascii "QSDFGHJKLM%"
	.byte '~,0,'#
	.ascii "WXCVBN?./\\"
	.byte 0,'*,0,32		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte '-,0,0,0,'+	/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '>
	.fill 10,1,0

alt_map:
	.byte 0,0
	.ascii "\0~#{[|`\\^@]}"
	.byte 0,0
	.byte '@,0,0,0,0,0,0,0,0,0,0
	.byte '~,13,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0		/* 36-39 */
	.fill 16,1,0		/* 3A-49 */
	.byte 0,0,0,0,0		/* 4A-4E */
	.byte 0,0,0,0,0,0,0	/* 4F-55 */
	.byte '|
	.fill 10,1,0

#else
#error "KBD-type not defined"
#endif
/*
 * do_self handles "normal" keys, ie keys that don't change meaning
 * and which have just one character returns.
 * 处理普通键。含义没有变化，并且只有一个字符返回的键
 */
do_self:
	lea alt_map,%ebx		# alt同时按下时的映射表基址。alt_map->ebx
	testb $0x20,mode		/* alt-gr。 右侧alt键按下 */
	jne 1f					# 没有，向前跳转到1号代码
	lea shift_map,%ebx		# shift键同时按下的映射表基址。 shift-》ebx
	testb $0x03,mode		# 右shift同时按下了吗
	jne 1f					# 有，向前跳转
	lea key_map,%ebx		# 使用普通映射表key_map
1:	movb (%ebx,%eax),%al	# 将扫描码作为索引值，取对应的ascii码到al中
	orb %al,%al				# 检测是否有对应的ascii码
	je none					# 没有，则返回
	testb $0x4c,mode		/* ctrl or caps。 ctlr或大小写键按下 */
	je 2f					# 跳转到2号标志处
	cmpb $'a,%al			# 将al中的字符与a比较
	jb 2f					# al<a , 则跳转到2号
	cmpb $'},%al			# al与｜比较
	ja 2f					# 如果大于｜，则跳转到2号标记
	subb $32,%al			# al减去32. 转换为大写。
2:	testb $0x0c,mode		/* ctrl */
	je 3f
	cmpb $64,%al			# 与@ 进行比较
	jb 3f					# 小于@，跳转到3号
	cmpb $64+32,%al			# al与96，` 反点
	jae 3f					# 大于96， 跳转到3号
	subb $64,%al			# 减去0x40， 转换为0x00-0x1f之间的字符
3:	testb $0x10,mode		/* left alt。 左侧alt键同时按下 */
	je 4f					# 没有，跳转到4号
	orb $0x80,%al			# al中位7置位
4:	andl $0xff,%eax			# 清eax的高子和ah
	xorl %ebx,%ebx			# 清ebx
	call put_queue			# 将字符放入缓冲队列中
none:	ret

/*
 * minus has a routine of it's own, as a 'E0h' before
 * the scan code for minus means that the numeric keypad
 * slash was pushed.
 * 减号有它自己的处理程序。因为减号扫描码前的0xe0意味着按下了数字小键盘上的斜杠键
 */
minus:	cmpb $1,e0		# e0 标志置位了吗？
	jne do_self			# 没有，调用do_self对减号进行普通处理
	movl $'/,%eax		# 否则用/替换减号
	xorl %ebx,%ebx		# 
	jmp put_queue		# 将字符放入缓冲队列中

/*
 * This table decides which routine to call when a scan-code has been
 * gotten. Most routines just call do_self, or none, depending if
 * they are make or break.
 * 字程序跳转表。 当取得扫描码后就根据此表相应的扫描码处理字程序
 * 大多数调用的子程序时do_self， 或者none。 取决于是按下还是松开。
 */
key_table:
	.long none,do_self,do_self,do_self	/* 00-03 s0 esc 1 2 */
	.long do_self,do_self,do_self,do_self	/* 04-07 3 4 5 6 */
	.long do_self,do_self,do_self,do_self	/* 08-0B 7 8 9 0 */
	.long do_self,do_self,do_self,do_self	/* 0C-0F + ' bs tab */
	.long do_self,do_self,do_self,do_self	/* 10-13 q w e r */
	.long do_self,do_self,do_self,do_self	/* 14-17 t y u i */
	.long do_self,do_self,do_self,do_self	/* 18-1B o p } ^ */
	.long do_self,ctrl,do_self,do_self	/* 1C-1F enter ctrl a s */
	.long do_self,do_self,do_self,do_self	/* 20-23 d f g h */
	.long do_self,do_self,do_self,do_self	/* 24-27 j k l | */
	.long do_self,do_self,lshift,do_self	/* 28-2B { para lshift , */
	.long do_self,do_self,do_self,do_self	/* 2C-2F z x c v */
	.long do_self,do_self,do_self,do_self	/* 30-33 b n m , */
	.long do_self,minus,rshift,do_self	/* 34-37 . - rshift * */
	.long alt,do_self,caps,func		/* 38-3B alt sp caps f1 */
	.long func,func,func,func		/* 3C-3F f2 f3 f4 f5 */
	.long func,func,func,func		/* 40-43 f6 f7 f8 f9 */
	.long func,num,scroll,cursor		/* 44-47 f10 num scr home */
	.long cursor,cursor,do_self,cursor	/* 48-4B up pgup - left */
	.long cursor,cursor,do_self,cursor	/* 4C-4F n5 right + end */
	.long cursor,cursor,cursor,cursor	/* 50-53 dn pgdn ins del */
	.long none,none,do_self,func		/* 54-57 sysreq ? < f11 */
	.long func,none,none,none		/* 58-5B f12 ? ? ? */
	.long none,none,none,none		/* 5C-5F ? ? ? ? */
	.long none,none,none,none		/* 60-63 ? ? ? ? */
	.long none,none,none,none		/* 64-67 ? ? ? ? */
	.long none,none,none,none		/* 68-6B ? ? ? ? */
	.long none,none,none,none		/* 6C-6F ? ? ? ? */
	.long none,none,none,none		/* 70-73 ? ? ? ? */
	.long none,none,none,none		/* 74-77 ? ? ? ? */
	.long none,none,none,none		/* 78-7B ? ? ? ? */
	.long none,none,none,none		/* 7C-7F ? ? ? ? */
	.long none,none,none,none		/* 80-83 ? br br br */
	.long none,none,none,none		/* 84-87 br br br br */
	.long none,none,none,none		/* 88-8B br br br br */
	.long none,none,none,none		/* 8C-8F br br br br */
	.long none,none,none,none		/* 90-93 br br br br */
	.long none,none,none,none		/* 94-97 br br br br */
	.long none,none,none,none		/* 98-9B br br br br */
	.long none,unctrl,none,none		/* 9C-9F br unctrl br br */
	.long none,none,none,none		/* A0-A3 br br br br */
	.long none,none,none,none		/* A4-A7 br br br br */
	.long none,none,unlshift,none		/* A8-AB br br unlshift br */
	.long none,none,none,none		/* AC-AF br br br br */
	.long none,none,none,none		/* B0-B3 br br br br */
	.long none,none,unrshift,none		/* B4-B7 br br unrshift br */
	.long unalt,none,uncaps,none		/* B8-BB unalt br uncaps br */
	.long none,none,none,none		/* BC-BF br br br br */
	.long none,none,none,none		/* C0-C3 br br br br */
	.long none,none,none,none		/* C4-C7 br br br br */
	.long none,none,none,none		/* C8-CB br br br br */
	.long none,none,none,none		/* CC-CF br br br br */
	.long none,none,none,none		/* D0-D3 br br br br */
	.long none,none,none,none		/* D4-D7 br br br br */
	.long none,none,none,none		/* D8-DB br ? ? ? */
	.long none,none,none,none		/* DC-DF ? ? ? ? */
	.long none,none,none,none		/* E0-E3 e0 e1 ? ? */
	.long none,none,none,none		/* E4-E7 ? ? ? ? */
	.long none,none,none,none		/* E8-EB ? ? ? ? */
	.long none,none,none,none		/* EC-EF ? ? ? ? */
	.long none,none,none,none		/* F0-F3 ? ? ? ? */
	.long none,none,none,none		/* F4-F7 ? ? ? ? */
	.long none,none,none,none		/* F8-FB ? ? ? ? */
	.long none,none,none,none		/* FC-FF ? ? ? ? */

/*
 * kb_wait waits for the keyboard controller buffer to empty.
 * there is no timeout - if the buffer doesn't empty, we hang.
 * 等待键盘控制器缓冲空，不存在超时处理。如果缓冲永远不为空的话。
 * 程序就永远等待
 */
kb_wait:
	pushl %eax
1:	inb $0x64,%al			# 读键盘控制器状态
	testb $0x02,%al			# 测试输入缓冲器是否为空，就是=0
	jne 1b					# 如果不为空，则循环等待。
	popl %eax				# 弹出eax
	ret
/*
 * This routine reboots the machine by asking the keyboard
 * controller to pulse the reset-line low.
 * 同座设置键盘控制器，向复位线输出负脉冲，使系统复位重启
 */
reboot:
	call kb_wait		# 等键键盘控制器输入缓冲器空
	movw $0x1234,0x472	/* don't do memory check */
	movb $0xfc,%al		/* pulse reset and A20 low */
	outb %al,$0x64		# 向系统复位和A20线输出负脉冲
die:	jmp die			# 死机
