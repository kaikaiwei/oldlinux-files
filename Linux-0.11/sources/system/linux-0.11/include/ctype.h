/**
 * 字符类型相关的操作和宏
 */
#ifndef _CTYPE_H
#define _CTYPE_H

#define _U	0x01	/* upper 大写 */
#define _L	0x02	/* lower 小写 */
#define _D	0x04	/* digit 数字 */
#define _C	0x08	/* cntrl 控制 */
#define _P	0x10	/* punct 标点字符*/
#define _S	0x20	/* white space (space/lf/tab) 空白字符，如空格，换行符 */
#define _X	0x40	/* hex digit 十六进制数字 */
#define _SP	0x80	/* hard space (0x20) 空格字符 */

extern unsigned char _ctype[]; // 字符特性数组
extern char _ctmp;      // 临时字符变量， lib/ctype.c中定义

// 字符或数字
#define isalnum(c) ((_ctype+1)[c]&(_U|_L|_D))
// 字符
#define isalpha(c) ((_ctype+1)[c]&(_U|_L))
// 控制字符
#define iscntrl(c) ((_ctype+1)[c]&(_C))
// 数字
#define isdigit(c) ((_ctype+1)[c]&(_D))
// 图形字符
#define isgraph(c) ((_ctype+1)[c]&(_P|_U|_L|_D))
// 小写字符
#define islower(c) ((_ctype+1)[c]&(_L))
// 可打印符号
#define isprint(c) ((_ctype+1)[c]&(_P|_U|_L|_D|_SP))
// 标点符号
#define ispunct(c) ((_ctype+1)[c]&(_P))
// 空格符
#define isspace(c) ((_ctype+1)[c]&(_S))
// 大写字符
#define isupper(c) ((_ctype+1)[c]&(_U))
// 十六进制数字
#define isxdigit(c) ((_ctype+1)[c]&(_D|_X))
// ascii字符
#define isascii(c) (((unsigned) c)<=0x7f)
// 转换成ascii字符
#define toascii(c) (((unsigned) c)&0x7f)

// 转换为小写字符
#define tolower(c) (_ctmp=c,isupper(_ctmp)?_ctmp-('A'-'a'):_ctmp)
// 转换为大写字符
#define toupper(c) (_ctmp=c,islower(_ctmp)?_ctmp-('a'-'A'):_ctmp)

#endif
