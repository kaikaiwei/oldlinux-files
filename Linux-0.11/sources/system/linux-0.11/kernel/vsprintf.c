/*
 *  linux/kernel/vsprintf.c
 *  用于对参数产生格式化输出
 *  (C) 1991  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#include <stdarg.h>
#include <string.h>

/* we use this so that we can do without the ctype library */
// 是否为数字
#define is_digit(c)	((c) >= '0' && (c) <= '9')

/**
 * 将字符数字转换成整型
 */
static int skip_atoi(const char **s)
{
	int i=0;

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

// 转换类型的符号常量
#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

// 除操作。 n是被除数，base是除数，n是商。函数返回值是余数
#define do_div(n,base) ({ \
int __res; \
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
__res; })

/**
 * 将整数转换为指定类型的字符串
 * @param str 输出
 * @param num 整数
 * @param base 进制
 * @param size	字符串长度
 * @param precision 整数字长度 精度
 * @param type 类型选项
 */
static char * number(char * str, int num, int base, int size, int precision
	,int type)
{
	char c,sign,tmp[36];
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	// 如果类型是小写字母
	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";

	// 如果靠左边界，则屏蔽填0标志
	if (type&LEFT) type &= ~ZEROPAD;

	// 进制基数小于2或者大于36。则退出
	if (base<2 || base>36)
		return 0;

	// 填0标志。
	c = (type & ZEROPAD) ? '0' : ' ' ;

	// 带符号标记
	if (type&SIGN && num<0) {
		sign='-';
		num = -num;
	} else
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);
	
	// 如果带符号，则宽度减去1
	if (sign) size--;

	// 如果是特殊类型，则对于十六进制宽度再减少2位（0x），对于爸进制宽度再减1（八进制换算结果前放1个0）
	if (type&SPECIAL)
		if (base==16) size -= 2;
		else if (base==8) size--;
	
	// 如果num为0，则临时字符串为‘0’
	i=0;
	if (num==0)
		tmp[i++]='0';
	else while (num!=0)
		tmp[i++]=digits[do_div(num,base)];

	// 若数值字符个数大于精度值，则精度值扩展为数字个数值。
	if (i>precision) precision=i;
	size -= precision;
	
	// 左对齐并且填充0。 那么在str中添加0
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			*str++ = ' ';
	
	// 如果有符号，那么添加符号
	if (sign)
		*str++ = sign;

	// 特殊类型的。 8进制和16进制，8进制添加0，16进制添加0x
	if (type&SPECIAL)
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	// 如果不是左侧填充。 写入c。0或者是空格
	if (!(type&LEFT))
		while(size-->0)
			*str++ = c;
	
	// 若数值字符个数大于精度值。 则存放精度值-i个0
	while(i<precision--)
		*str++ = '0';
	// 将数值转换好的字符串写入str中。
	while(i-->0)
		*str++ = tmp[i];
	// 若宽度值仍然大于0， 表示类型标志中有做烤漆标志，则在剩下的宽度中放入空格
	while(size-->0)
		*str++ = ' ';
	return str;
}

/**
 * 格式化输出
 * @param buf 输出到的缓存区
 * @param fmt 格式
 * @param va_list args类型的无限制个数参数
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	int i;
	char * str;	// 存放转换过程中的字符串
	char *s;
	int *ip;

	int flags;		/* flags to number()  number函数使用标志 */

	int field_width;	/* width of output field 输出字段的宽度*/
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
				   		// min 整数数字个数
						// max 字符串的字符个数
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	// 首先将字符指针指向buf，然后扫描格式字符串，对各个格式转换只是进行相应的处理
	for (str=buf ; *fmt ; ++fmt) {
		// 对于非%号开头的，跳过。
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}
			
		// 处理flag标志。先置0，再使用
		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */ // 跳过第一个%号
			// % 后面的标志为处理。
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}
		
		/* get field width， 宽度处理 */
		field_width = -1;
		if (is_digit(*fmt))		// 如果是数字，则直接取宽度值
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {		// 如果宽度域为*号，则表示下一个参数指定宽度，调用va_arg取宽度值
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		// 取格式转换的精度。
		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;	
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		// 分析长度修饰符，并放在qualifier中
		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		// 转换指示符。
		switch (*fmt) {
		case 'c':		// 对应参数是字符。
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			// 取出参数中字符。写入str数组中
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;

		case 's':	// 对应参数是字符串
			s = va_arg(args, char *);	// 取出字符串
			len = strlen(s);			// 获取字符串长度
			if (precision < 0)			// 如果精度值小于0
				precision = len;
			else if (len > precision)	// 截取
				len = precision;

			// 不是做对齐，填充空格
			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			// 写入字符串
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			// 填充空格
			while (len < field_width--)
				*str++ = ' ';
			break;

		case 'o':			// 表示这是一个8进制。
			str = number(str, va_arg(args, unsigned long), 8,
				field_width, precision, flags);
			break;

		case 'p':			// 这是一个指针类型的
			if (field_width == -1) {
				field_width = 8;
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			break;

		case 'x':	// 16进制小写
			flags |= SMALL;
		case 'X':	// 16进制大些
			str = number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		case 'd':	// 整数类型
		case 'i':
			flags |= SIGN;
		case 'u':
			str = number(str, va_arg(args, unsigned long), 10,
				field_width, precision, flags);
			break;

		case 'n':	// 把到目前为止转换输出的字符串保存到对应参数指针对应的位置。
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;

		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}
	*str = '\0';	// 字符串结尾
	return str-buf;	// 返回长度
}
