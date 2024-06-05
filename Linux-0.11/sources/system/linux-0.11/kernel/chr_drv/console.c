/*
 *  linux/kernel/console.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 * Beeping thanks to John T Kohl.  感谢John实现的蜂鸣提示
 * 实现控制台输入输出功能。 con_init和con_write
 * 
 */

/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */

/*
 * Code to check for different video-cards mostly by Galen Hunt,
 * <g-hunt@ee.utah.edu>
 */

#include <linux/sched.h>	// 调度
#include <linux/tty.h>		// 终端
#include <asm/io.h>			// 输出输出
#include <asm/system.h>		// 系统头文件。设置或修改描述符/中断门等的嵌入式宏汇编

/*
 * These are set up by the setup-routine at boot-time:
 * setup在引导程序启动时设置的参数
 */
// 光标列号
#define ORIG_X			(*(unsigned char *)0x90000)
// 光标行号
#define ORIG_Y			(*(unsigned char *)0x90001)
// 显示页面
#define ORIG_VIDEO_PAGE		(*(unsigned short *)0x90004)
// 显示模式
#define ORIG_VIDEO_MODE		((*(unsigned short *)0x90006) & 0xff)
// 字符列数
#define ORIG_VIDEO_COLS 	(((*(unsigned short *)0x90006) & 0xff00) >> 8)
// 显示行数
#define ORIG_VIDEO_LINES	(25)
 
#define ORIG_VIDEO_EGA_AX	(*(unsigned short *)0x90008)
// 显示内存大小和色彩模式
#define ORIG_VIDEO_EGA_BX	(*(unsigned short *)0x9000a)
// 显卡特性参数
#define ORIG_VIDEO_EGA_CX	(*(unsigned short *)0x9000c)

#define VIDEO_TYPE_MDA		0x10	/* Monochrome Text Display。 单色文本	*/
#define VIDEO_TYPE_CGA		0x11	/* CGA Display 		CGA显示器	*/
#define VIDEO_TYPE_EGAM		0x20	/* EGA/VGA in Monochrome Mode 单色	*/
#define VIDEO_TYPE_EGAC		0x21	/* EGA/VGA in Color Mode  彩色模式	*/

#define NPAR 16

// 键盘中断处理函数
extern void keyboard_interrupt(void);

static unsigned char	video_type;		/* Type of display being used  显示类型	*/
static unsigned long	video_num_columns;	/* Number of text columns  显示列数	*/
static unsigned long	video_size_row;		/* Bytes per row	 显示行数	*/
static unsigned long	video_num_lines;	/* Number of test lines	 每行使用的字节数	*/
static unsigned char	video_page;		/* Initial video page   初始显示页面		*/
static unsigned long	video_mem_start;	/* Start of video RAM  显示内存起始地址		*/
static unsigned long	video_mem_end;		/* End of video RAM (sort of)  显示内存结束地址	*/
static unsigned short	video_port_reg;		/* Video register select port  显示控制索引寄存器端口	*/
static unsigned short	video_port_val;		/* Video register value port  显示控制数据寄存器端口	*/
static unsigned short	video_erase_char;	/* Char+Attrib to erase with  擦除字符属性与字符 0x0720	*/

// 用于屏幕卷屏操作
static unsigned long	origin;		/* Used for EGA/VGA fast scroll 用于快速滚屏，滚屏开始内存地址	*/
static unsigned long	scr_end;	/* Used for EGA/VGA fast scroll 滚屏末端内存地址	*/
static unsigned long	pos;		// 当前光标对应的显示内存位置
static unsigned long	x,y;		// 当前光标位置
static unsigned long	top,bottom;	// 滚动时顶行行号，底行行号
static unsigned long	state=0;	// ANSI转移字符序列处理窗台
static unsigned long	npar,par[NPAR];	// ANSI转移字符序列参数个数和参数数组
static unsigned long	ques=0;			// 
static unsigned char	attr=0x07;		// 字符属性，黑底白字

static void sysbeep(void);	// 系统蜂鸣函数。 本文末尾

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 * 终端回应SC-Z or csi0c（vt100）请求的应答。
 */
#define RESPONSE "\033[?1;2c"

/* NOTE! gotoxy thinks x==video_num_columns is ok 
 * gotoxy函数认为 x==video_num_colums，
 */
static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
	// 如果输入的坐标超出了显示器列数。或光标行号超出显示的最大行。退出。
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	x=new_x;
	y=new_y;
	// 计算pos
	pos=origin + y*video_size_row + (x<<1);
}

/**
 * 设置滚屏起始显示内存地址
 */
static inline void set_origin(void)
{
	cli();	// 关中断
	outb_p(12, video_port_reg);	// r12，显示控制数据寄存器
	// 写入卷屏起始地址高字节。
	// 向右移动9位，表示想右移动8位，在除以2（2字节代表屏幕上1个字符），数相对于默认显示内存操作的
	outb_p(0xff&((origin-video_mem_start)>>9), video_port_val);	
	// r13，显示控制数据寄存器。 写入卷平起始地址底字节。向右移动1位表示除以2.
	outb_p(13, video_port_reg);
	outb_p(0xff&((origin-video_mem_start)>>1), video_port_val);
	sti();
}

/**
 * 向上卷动一行（屏幕窗口向下移动）。
 * 将屏幕窗口向下移动一行。
 */
static void scrup(void)
{
	// EGA显示模式。
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		// top =0， bottom=video_num_lines=25，表示整屏窗口向下移动
		if (!top && bottom == video_num_lines) {
			// 调整屏幕显示对应的内存地址
			origin += video_size_row;	// 增加显示行数
			pos += video_size_row;		// 当前位置
			scr_end += video_size_row;	// 结束内存增加
			// 屏幕末端最后一个显示字符对应的内存指针dcr_end超出显示内存的末端。
			// 将屏幕内容数据移动到显示内存的起始位置，video_mem_start处，并在出现的新行上填充空格字符
			if (scr_end > video_mem_end) {
				__asm__("cld\n\t"		// 清方向位
					"rep\n\t"			// 重复操作
					"movsl\n\t"			// 移动到显示内存起始处
					"movl _video_num_columns,%1\n\t"	// ecx=1， 行字符数
					"rep\n\t"							// 重复操作
					"stosw"								// 填入插入字符，空格字符
					::"a" (video_erase_char),			// 擦除字符
					"c" ((video_num_lines-1)*video_num_columns>>1),	// 
					"D" (video_mem_start),				// 显示内存起始位置
					"S" (origin)						// 滚屏开始内存地址
					:"cx","di","si");
				
				// 更新指针。
				scr_end -= origin-video_mem_start;	// 屏幕末端地址
				pos -= origin-video_mem_start;		// 当前位置
				origin = video_mem_start;			// 滚屏显示地址
			} else {
				__asm__("cld\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),		// 擦除字符
					"c" (video_num_columns),		// 显示列数
					"D" (scr_end-video_size_row)	// 起始地址
					:"cx","di");
			}
			// 重置起始位置
			set_origin();		 
		} else {
			__asm__("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl _video_num_columns,%%ecx\n\t"
				"rep\n\t"
				"stosw"
				::"a" (video_erase_char),
				"c" ((bottom-top-1)*video_num_columns>>1),
				"D" (origin+video_size_row*top),
				"S" (origin+video_size_row*(top+1))
				:"cx","di","si");
		}
	}
	else		/* Not EGA/VGA */
	{
		__asm__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*top),
			"S" (origin+video_size_row*(top+1))
			:"cx","di","si");
	}
}

/**
 * 向下卷动一行
 */
static void scrdown(void)
{
	// ega显示模式
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		__asm__("std\n\t"		// 方向位
			"rep\n\t"			// 重复指令
			"movsl\n\t"			// 进行移动
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4。 每次减少4字节 */
			"movl _video_num_columns,%%ecx\n\t"	// 显示列数到ecx中
			"rep\n\t"							// 重复指令
			"stosw"								// 填入插入字符
			::"a" (video_erase_char),			// 没有输出寄存器，只有输入寄存器
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:"ax","cx","di","si");
	}
	else		/* Not EGA/VGA */
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:"ax","cx","di","si");
	}
}

/**
 * 光标下移一行。
 * lf ： line feed，换行
 */
static void lf(void)
{
	// 如果没有在第二行之后
	if (y+1<bottom) {		
		y++;	// 直接修改光标当前行变量
		pos += video_size_row;	// 光标对应的显示内存的位置
		return;
	}
	// 否则，需要将屏幕内容上移一行
	scrup();
}

/**
 * 光标上移一行
 * ri ： reverse line feed ，反向换行
 */
static void ri(void)
{
	// 如果光标不在第一行上
	if (y>top) {
		y--; // 直接修改光标当前行变量
		pos -= video_size_row;	// 光标对应的显示内存位置
		return;
	}
	// 否则将屏幕内容下移一行
	scrdown();
}

/**
 * 光标回到第一列左端
 * cr ： carriage return 回车
 */
static void cr(void)
{
	pos -= x<<1; // 调整光标对应的显示内存位置
	x=0;	// x 清0
}

/**
 * 擦除光标前一个字符，空格代替
 */
static void del(void)
{
	if (x) {
		pos -= 2; // 内存位置
		x--;	  // x调整
		*(unsigned short *)pos = video_erase_char; // 对应的pos填充空格
	}
}

/**
 * 删除屏幕上与光标位置相关的部分
 * csi 控制序列引导码， Control Sequence Introducer。
 * ANSI 转移序列 ‘ESC [sJ'  
 * 0: 删除光标到屏幕底部。
 * 1: 删除屏幕开始到光标处
 * 2: 整屏删除
 * par 对应上面的s
 */
static void csi_J(int par)
{
	// 设为寄存器变量
	long count __asm__("cx");
	long start __asm__("di");

	// 根据par的值，左不同的操作
	switch (par) {
		case 0:	/* erase from cursor to end of display  删除光标到结尾处的内容*/
			count = (scr_end-pos)>>1;
			start = pos;	// 开始位置
			break;
		case 1:	/* erase from start to cursor 。 删除开始到光标处的内容*/
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display 删除整个内容 */
			count = video_num_columns * video_num_lines;
			start = origin;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"	// 方向置位
		"rep\n\t"		// 重复指令
		"stosw\n\t"		// 填充空格
		::"c" (count),	// ecx=count， 
		"D" (start),"a" (video_erase_char) // edx=start， eax=空格
		:"cx","di");
}

/**
 * 删除行内与光标相关的部分，以一行位单位。
 * ANSI 转移序列 ‘ESC [sK'  
 * 0: 删除到行位
 * 1: 从开始删除
 * 2: 整行都删除
 */
static void csi_K(int par)
{
	// 使用寄存器
	long count __asm__("cx");
	long start __asm__("di");

	switch (par) {
		case 0:	/* erase from cursor to end of line */
			if (x>=video_num_columns)
				return;
			count = video_num_columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<video_num_columns)?x:video_num_columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = video_num_columns;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:"cx","di");
}

/**
 * 允许翻译（重显）。 允许重新设置字符显示方式。 比如加粗，加下划线，闪烁，反显等。
 */
void csi_m(void)
{
	int i;

	for (i=0;i<=npar;i++)
		switch (par[i]) {
			case 0:attr=0x07;break;	// 正常显示
			case 1:attr=0x0f;break;	// 加粗
			case 4:attr=0x0f;break;	// 加下划线
			case 7:attr=0x70;break;	// 反显
			case 27:attr=0x07;break;	// 正常显示
		}
}

/**
 * 设置光标位置
 */
static inline void set_cursor(void)
{
	cli(); // 关中断
	outb_p(14, video_port_reg);	//使用索引寄存器端口选择显示控制数据寄存器r14. 光标当前显示位置高字节
	outb_p(0xff&((pos-video_mem_start)>>9), video_port_val);	// 写入光标当前位置的高字节。
	outb_p(15, video_port_reg);									// 索引寄存器选择r15， 并将光标当前位置低字节写入
	outb_p(0xff&((pos-video_mem_start)>>1), video_port_val);
	sti();	// 开中断
}

/**
 * 发送对中断VT100的相应序列。将相应序列放入读缓冲队列中
 */
static void respond(struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();	// 关中断
	// 将字符队列放入写队列
	while (*p) {
		PUTCH(*p,tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);	// 转换为规范模式（放入到辅助队列中）
}

/**
 * 在光标处插入一空格字符
 */
static void insert_char(void)
{
	int i=x;
	unsigned short tmp, old = video_erase_char;
	unsigned short * p = (unsigned short *) pos;

	while (i++<video_num_columns) {
		tmp=*p;	// 临时保存
		*p=old;	// 当前位置插入空格字符
		old=tmp;	// old保存之前的字符
		p++;		// 递增当前位置
	}
}

/**
 * 插入一行空格
 */
static void insert_line(void)
{
	int oldtop,oldbottom;

	// 保存原top bottom的值
	oldtop=top;
	oldbottom=bottom;

	top=y;
	bottom = video_num_lines;
	scrdown();	// 向下移动一行

	// 更新top和bottom
	top=oldtop;
	bottom=oldbottom;
}

/**
 * 删除光标所处位置的一个字符
 */
static void delete_char(void)
{
	int i;
	unsigned short * p = (unsigned short *) pos;	// 当前位置

	// 超出屏幕最右端，返回
	if (x>=video_num_columns)
		return;
	// 从光标处开始
	i = x;
	// 每一个都依次向前移动
	while (++i < video_num_columns) {
		*p = *(p+1);
		p++;
	}
	// 最后一个填入空字符
	*p = video_erase_char;
}

/**
 * 删除光标所在行
 */
static void delete_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom = video_num_lines;
	scrup(); // 向上卷屏
	top=oldtop;
	bottom=oldbottom;
}

/**
 * 在光标处插入nr个字符
 */
static void csi_at(unsigned int nr)
{
	// 超过屏幕右端
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;

	//  插入字符
	while (nr--)
		insert_char();
}

/**
 * 在光标处插入nr行。 ANSI转移字符序列‘ESC [nL‘
 */
static void csi_L(unsigned int nr)
{
	// 插入的行数大于屏幕最多行数。则截为屏幕显示行数
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)	// 若插入行数为0， 则插入1行
		nr = 1;
	while (nr--)
		insert_line();
}

/**
 * 删除光标处的nr个字符。 ANSI转移字符序列‘ESC [nP‘
 */
static void csi_P(unsigned int nr)
{
	// 阐述行数大于屏幕最大列数
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		delete_char();
}

/**
 * 删除光标处的nr行。 ANSI转移字符序列‘ESC [nM‘
 */
static void csi_M(unsigned int nr)
{
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line();
}

// 保留当前光标
static int saved_x=0;
static int saved_y=0;

/**
 * 保存当前光标位置
 */
static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}

/**
 * 恢复保存的光标位置
 */
static void restore_cur(void)
{
	gotoxy(saved_x, saved_y);
}

/**
 * 控制台写函数。 从终端对应的tty写缓冲队列中取字符，并显示在屏幕上。
 */
void con_write(struct tty_struct * tty)
{
	int nr;
	char c;

	// 取得写缓冲队列中现有字符数nr，然后针对每个字符进行处理
	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q,c); // 从写队列中获取一个字符到变量c中
		/*
		 * 状态处理
		 * 0: 初始状态
		 * 1: 
		 */
		switch(state) {
			case 0:
				// 如果不是控制字符（小于31），也不是扩展字符（大于127）
				if (c>31 && c<127) {
					// 当前光标位置行末端。将光标移动到下一行的行首。 并调整光标位置对应的内存指针pos。
					if (x>=video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();	// 光标下移一行
					}
					__asm__("movb _attr,%%ah\n\t"
						"movw %%ax,%1\n\t"
						::"a" (c),"m" (*(short *)pos)
						:"ax");
					pos += 2;	// 当前位置+2
					x++;		// x坐标增加
				} else if (c==27)	// 如果是转移字符，则状态修改为1
					state=1;
				else if (c==10 || c==11 || c==12) // 如果是10 换行符，11 垂直制表符VT，12 FF 换页符
					lf();	// 光标移动到下一行
				else if (c==13)	// 回车符CR。 将光标移动到头列
					cr();
				else if (c==ERASE_CHAR(tty))	// 如果是DEL 127符号，则将光标右侧以字符擦除，并移动光标
					del();
				else if (c==8) {	// 如果是BS（Backspace， 8） 光标左移一位，并调整光标对应内存位置。
					if (x) {
						x--;
						pos -= 2;
					}
				} else if (c==9) {  // 如果是TAB，水平制表符。 将光标移动到8的倍数列上
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>video_num_columns) {	// 若超出最大列数，则将光标移动到下一行上
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					c=9;
				} else if (c==7)	// 响铃符 BEL 
					sysbeep();
				break;
			case 1:			// 原状态0，字符是转移字符ESC（0x1b=33=27）
				state=0;
				if (c=='[')
					state=2;
				else if (c=='E')
					gotoxy(0,y+1); // 光标移动到下一行
				else if (c=='M')	// 光标上移一行
					ri();
				else if (c=='D')	// 光标下移一行
					lf();
				else if (c=='Z')	// 发送中断应答字符序列
					respond(tty);
				else if (x=='7')	// 保存当前光标位置。 应该是c不是x
					save_cur();
				else if (x=='8')	// 回复保存的光标位置。 应该是c不是x
					restore_cur();
				break;
			case 2:
				// 对esc转移字符序列使用处理数组par清0.
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;	// 指向首项
				state=3; // 修改状态为3
				if (ques=(c=='?'))	// 字符不是？，直接到状态3处理
					break;
			case 3:	// 原来是状态2，或者原来是状态3，但是原字符是；或数字。
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0'; // 数字处理
					break;
				} else state=4;	// 跳转到状态4
			case 4:
				state=0;	// 回到原状态
				switch(c) {
					case 'G': case '`':
						if (par[0]) par[0]--;
						gotoxy(par[0],y); // par第一个表示列数。
						break;
					case 'A':
						if (!par[0]) par[0]++;		// 光标上移行数
						gotoxy(x,y-par[0]);
						break;
					case 'B': case 'e':				// 光标下移行数
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					case 'C': case 'a':				// 光标右移列数
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					case 'D':					// 光标左移动列数
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					case 'E':				// 光标向下移动的行数，并移动到首列
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					case 'F':				// 光标上移行数，并回到0列
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					case 'd':
						if (par[0]) par[0]--;	// 光标需要跳转到的行号
						gotoxy(x,par[0]);
						break;
					case 'H': case 'f':		// 表表行号，第二个参数代表列号
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					case 'J':
						csi_J(par[0]); // 清屏
						break;
					case 'K':
						csi_K(par[0]);	// 删除处理
						break;
					case 'L':
						csi_L(par[0]);	// 插入n行
						break;
					case 'M':
						csi_M(par[0]);	// 删除n行
						break;
					case 'P':
						csi_P(par[0]);	// 删除n个字符
						break;
					case '@':
						csi_at(par[0]);	// 插入n个字符。 ansi转义字符序列
						break;
					case 'm':			// 改变光标处的显示属性
						csi_m();
						break;
					case 'r':		// 滚屏的起始行号和终止行号
						if (par[0]) par[0]--;
						if (!par[1]) par[1] = video_num_lines;
						if (par[0] < par[1] &&
						    par[1] <= video_num_lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					case 's':
						save_cur();	// 保存当前光标位置
						break;
					case 'u':
						restore_cur();	// 回复光标到原来位置
						break;
				}
		}
	}
	set_cursor();	// 根据上面设置的光标位置，向显示控制器发送光标显示位置。
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 *
 * Reads the information preserved by setup.s to determine the current display
 * type and sets everything accordingly.
 * 初始化控制台中断。
 */
void con_init(void)
{
	register unsigned char a;
	char *display_desc = "????";
	char *display_ptr;

	video_num_columns = ORIG_VIDEO_COLS;		// 显示器显示字符列数
	video_size_row = video_num_columns * 2;		// 每行需要的字节数
	video_num_lines = ORIG_VIDEO_LINES;			// 显示器显示字符的行数
	video_page = ORIG_VIDEO_PAGE;				// 当前显示页面
	video_erase_char = 0x0720;					// 查处字符。 0x20时显示。 0x07是属性
	
	// 显示模式等于7， 则表示是单色显示器
	if (ORIG_VIDEO_MODE == 7)			/* Is this a monochrome display? */
	{
		video_mem_start = 0xb0000;	// 设置单显映像内存起始地址
		video_port_reg = 0x3b4;		// 设置单显索引寄存器端口
		video_port_val = 0x3b5;		// 设置单显数据寄存器端口
		// 根据bios中断int 0x10功能 0x12获得的显示模式信息。 判断显示卡是单色还是彩色显示卡
		// 如果上述中断功能所得到的BX寄存器返回值不等于0x10， 说明是EGA显卡。
		// 初始化类型设置为EGA单色；所使用的地址是0xb0000。并置显示器描述字符串为EGAm。
		// 载系统初始化期间显示器描述字符串将显示载屏幕的右上角。
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAM;
			video_mem_end = 0xb8000;
			display_desc = "EGAm";
		}
		else
		{	
			// 单色显示卡MBDA。 
			video_type = VIDEO_TYPE_MDA;	// 显示器类型，MDA单色
			video_mem_end	= 0xb2000;		// 显示内存末端地址
			display_desc = "*MDA";			// 显示器描述字符串
		}
	}
	else								/* If not, it is color. */
	{
		// 彩色模式
		video_mem_start = 0xb8000;	// 显示内存开始地址
		video_port_reg	= 0x3d4;		// 显卡索引寄存器端口地址
		video_port_val	= 0x3d5;		// 显卡数据寄存器端口地址
		// 判断显卡类型。 BX不等于10，是EGA显卡
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAC;
			video_mem_end = 0xbc000;
			display_desc = "EGAc";
		}
		else
		{
			video_type = VIDEO_TYPE_CGA;	// 显示器类型。
			video_mem_end = 0xba000;	// 显示内存末端地址
			display_desc = "*CGA";	// CGA显卡
		}
	}

	/* Let the user known what kind of display driver we are using */
	// 让用户知道我们正在使用哪一类显示驱动程序
	
	// 在屏幕右上角显示显示描述字符串。
	display_ptr = ((char *)video_mem_start) + video_size_row - 8; // 屏幕最右端-4个字符
	while (*display_desc)
	{
		*display_ptr++ = *display_desc++;
		display_ptr++;
	}
	
	/* Initialize the variables used for scrolling (mostly EGA/VGA)	*/
	// 初始化用于滚屏的信息
	
	origin	= video_mem_start;
	scr_end	= video_mem_start + video_num_lines * video_size_row;
	top	= 0;			// 最顶端行号
	bottom	= video_num_lines; // 最底端行号

	gotoxy(ORIG_X,ORIG_Y);	// 移动光标位置。
	set_trap_gate(0x21,&keyboard_interrupt);	// 设置键盘中断陷阱门
	outb_p(inb_p(0x21)&0xfd,0x21);			// 取消8059A中对键盘的额屏幕。允许IRQ1
	a=inb_p(0x61);							// 延迟读取键盘端口0x61， 8025端口PB
	outb_p(a|0x80,0x61);					// 设置禁止键盘工作。 位7置位
	outb(a,0x61);							// 允许键盘工作。用来复位键盘操作
}
/* from bsd-net-2: */

/**
 * 停止蜂鸣器
 */
void sysbeepstop(void)
{
	/* disable counter 2 */
	outb(inb_p(0x61)&0xFC, 0x61); // 进制定时器2
}

int beepcount = 0;

/**
 * 开通蜂鸣器
 */
static void sysbeep(void)
{
	/* enable counter 2 */
	outb_p(inb_p(0x61)|3, 0x61);	// 开定时器2
	/* set command for counter 2, 2 byte write */
	outb_p(0xB6, 0x43);			// 送设置定时器2的命令
	/* send 0x637 for 750 HZ */
	outb_p(0x37, 0x42);			// 设置频率位750HZ， 因此送定时器0x637
	outb(0x06, 0x42);
	/* 1/8 second */
	beepcount = HZ/8;		// 蜂鸣时间设置为1/8秒
}
