#ifndef _STRING_H_
#define _STRING_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

extern char * strerror(int errno);

/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		(C) 1991 Linus Torvalds
 */
 
 /**
  * 字符串拷贝函数。
  */
extern inline char * strcpy(char * dest,const char *src)
{
	// 来源时src， 
__asm__("cld\n"			// 清方向位
	"1:\tlodsb\n\t"		// lodsb  加载ds：esi 处1字节到al， 并更新esi
	"stosb\n\t"			// stosb  存储指令。 保存al 到es：edi 并更新edi
	"testb %%al,%%al\n\t"	// 刚存储的字节是0
	"jne 1b"				// 不是则向后跳转到标号1处
	::"S" (src),"D" (dest):"si","di","ax");
return dest;
}

/**
 * 将源字符串拷贝到目的字符串
 * 如果源字符串长度小于count个字节，就附加NULL到目的字符串
 */
extern inline char * strncpy(char * dest,const char *src,int count)
{
__asm__("cld\n"			// 清方向位
	"1:\tdecl %2\n\t"	// ecx --
	"js 2f\n\t"			// 如果count < 0 , 跳转到标号2
	"lodsb\n\t"				// 加载ds：esi的1字节到al， 并更新esi++
	"stosb\n\t"				// 存储到ds：edi， edi++
	"testb %%al,%%al\n\t"	// 判断刚存储的是否为0
	"jne 1b\n\t"			// 不是则继续执行复制
	"rep\n\t"				// 否则，在目的串中放入剩余个数的空字符
	"stosb\n"				// 存储到ds：edi， edi++
	"2:"
	::"S" (src),"D" (dest),"c" (count):"si","di","ax","cx");
return dest;
}

/**
 * 将源字符串拷贝到目的字符串的末尾处
 */
extern inline char * strcat(char * dest,const char * src)
{
__asm__("cld\n\t"		// 清方向位
	"repne\n\t"			// 比较al和ds:edi字节， 并更新edi， 直到找到0
	"scasb\n\t"			// 这个时候edi指向后面1字节
	"decl %1\n"			// es:edi减去·
	"1:\tlodsb\n\t"		// ds:esi -> al, esi ++
	"stosb\n\t"			// al -> ds:edi, edi ++
	"testb %%al,%%al\n\t"	// ga那个写入的是否为0， 如果不是0， 则返回到标号1继续执行
	"jne 1b"
	::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):"si","di","ax","cx");
return dest;
}

/**
 * 在源字符串的count个字节复制到目的字符串的末尾出，最后添加一空字符
 * 
 */
extern inline char * strncat(char * dest,const char * src,int count)
{
__asm__("cld\n\t"	// 清方向位
	"repne\n\t"		// 重复指令
	"scasb\n\t"		// 比较al和es:edi字节，edi++, 知道找到目的串的末尾0字节
	"decl %1\n\t"	// ds:edi -- ， edi指向该0字节
	"movl %4,%3\n"	// 欲复制字节数->ecx
	"1:\tdecl %3\n\t"	// ecx-- 开始计数
	"js 2f\n\t"			// ecx < 0, 跳转到标号2
	"lodsb\n\t"			// ds:esi -> al, esi++
	"stosb\n\t"			// al->ds:edi, edi++
	"testb %%al,%%al\n\t"	// al 是否为0
	"jne 1b\n"				// 如果不为0， 继续拷贝，否则项强
	"2:\txorl %2,%2\n\t"	// 将al清0
	"stosb"					// 保存es:edi处
	::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
	:"si","di","ax","cx");
return dest;
}

/**
 * 将一个字符串和另外一个字符串进行比较
 * 
 */
extern inline int strcmp(const char * cs,const char * ct)
{
register int __res __asm__("ax");	// 结果变量
__asm__("cld\n"				// 清方向位
	"1:\tlodsb\n\t"			// 取字符串2的字节 ds:esi -> al, esi++
	"scasb\n\t"				// al与字符串1的es:edi 做比较，edi++
	"jne 2f\n\t"			// 如果不想等，跳转到标号2
	"testb %%al,%%al\n\t"	// al为0，表示到达结尾
	"jne 1b\n\t"			// 不是0，继续比较（向后跳转到标号1）
	"xorl %%eax,%%eax\n\t"	// 是，eax为0
	"jmp 3f\n"				// 向前跳转到标号3， 结束
	"2:\tmovl $1,%%eax\n\t"	// eax=1
	"jl 3f\n\t"				// 若前面比较中串2<串1， 则返回正值， 结束
	"negl %%eax\n"			// 否则eax=-eax， 返回负值，结束
	"3:"					// 
	:"=a" (__res):"D" (cs),"S" (ct):"si","di");
return __res;
}

/**
 * 字符串的指定count个字符对比。
 */
extern inline int strncmp(const char * cs,const char * ct,int count)
{
register int __res __asm__("ax");
__asm__("cld\n"				// 清方向位
	"1:\tdecl %3\n\t"		// count--
	"js 2f\n\t"				// 如果count < 0, 向前跳转到标号2
	"lodsb\n\t"				// ds:esi -> al, esi++, 取串2的字符到al
	"scasb\n\t"				// 比较al与串1的字符，es:edi, 并且edi++
	"jne 3f\n\t"			// 如果不想等，跳转到标号3的位置
	"testb %%al,%%al\n\t"	// al是否为0，是否到字符串结尾
	"jne 1b\n"				// 如果不是，继续比较，跳转到标号1
	"2:\txorl %%eax,%%eax\n\t"	// eax清0	
	"jmp 4f\n"					// 向前跳转到标号4，结束
	"3:\tmovl $1,%%eax\n\t"		// eax=1
	"jl 4f\n\t"					// 如果串2<串1， 返回1，结束
	"negl %%eax\n"				// 否则返回-1
	"4:"
	:"=a" (__res):"D" (cs),"S" (ct),"c" (count):"si","di","cx");
return __res;
}

/**
 * 在字符串中寻找的第一个匹配的字符
 */
extern inline char * strchr(const char * s,char c)
{
register char * __res __asm__("ax");
__asm__("cld\n\t"				// 清方向位
	"movb %%al,%%ah\n"			// 将欲比较的字符串放到ah中
	"1:\tlodsb\n\t"				// ds:esi -> al, esi ++
	"cmpb %%ah,%%al\n\t"		// 比较ah和al
	"je 2f\n\t"					// 相等则跳转到标号2
	"testb %%al,%%al\n\t"		// 不想等，al是否为0
	"jne 1b\n\t"				// 不为0， 继续比较
	"movl $1,%1\n"				// eax=1
	"2:\tmovl %1,%0\n\t"		// esi = 1， 
	"decl %0"					// 减少edi
	:"=a" (__res):"S" (s),"0" (c):"si");
return __res;
}

/**
 * 寻找字符串中指定字符最后一次出现的地方。反向搜索
 */
extern inline char * strrchr(const char * s,char c)
{
register char * __res __asm__("dx");	// 结果值
__asm__("cld\n\t"						// 清方向位
	"movb %%al,%%ah\n"					// al->ah
	"1:\tlodsb\n\t"						// ds:esi->al, esi ++
	"cmpb %%ah,%%al\n\t"				// 比较al和ah
	"jne 2f\n\t"						// 如果不想等，向前跳转到2号位置
	"movl %%esi,%0\n\t"					// 将字符指针保存在edx中
	"decl %0\n"							// edx减1
	"2:\ttestb %%al,%%al\n\t"			// 测试al是否为0
	"jne 1b"							// 不为0，继续查找
	:"=d" (__res):"0" (0),"S" (s),"a" (c):"ax","si");
return __res;
}

/**
 * 在cs中寻找ct的序列
 */
extern inline int strspn(const char * cs, const char * ct)
{
register char * __res __asm__("si");	// 结果指针
__asm__("cld\n\t"				// 清方向位
	"movl %4,%%edi\n\t"			// ct放入edi中
	"repne\n\t"					// 重复执行
	"scasb\n\t"					// 比较al与串2中的字符es:edi, edi++
	"notl %%ecx\n\t"			// 不想等就继续比较
	"decl %%ecx\n\t"			// ecx--
	"movl %%ecx,%%edx\n"		// 将ct的床度放入到edx中
	"1:\tlodsb\n\t"				// 取cs的字符ds:esi->al, esi++
	"testb %%al,%%al\n\t"		// 是否为0， 到串1的结尾
	"je 2f\n\t"					// 是的话跳转到2号标志
	"movl %4,%%edi\n\t"			// 串2的头指针放入edi中
	"movl %%edx,%%ecx\n\t"		// 串2的长度值放入到ecx中
	"repne\n\t"					// 比较al与串2字符es:edi, edi++
	"scasb\n\t"					// 不想等继续比较
	"je 1b\n"					// 想等，跳转到1号标记
	"2:\tdecl %0"				// esi--，指向最后一个包含在串2中的字符
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res-cs;
}

/**
 * 寻找字符串1中不包含字符串2中任何字符的首个字符序列
 */
extern inline int strcspn(const char * cs, const char * ct)
{
register char * __res __asm__("si");
__asm__("cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 1b\n"
	"2:\tdecl %0"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res-cs;
}

/**
 * 在字符串1中寻找包含在字符串2中的任何字符
 */
extern inline char * strpbrk(const char * cs,const char * ct)
{
register char * __res __asm__("si");
__asm__("cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 1b\n\t"
	"decl %0\n\t"
	"jmp 3f\n"
	"2:\txorl %0,%0\n"
	"3:"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res;
}

/**
 * 在字符串1中，寻找首个匹配整个字符串2的字符串
 */
extern inline char * strstr(const char * cs,const char * ct)
{
register char * __res __asm__("ax");
__asm__("cld\n\t" \
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
	"movl %%ecx,%%edx\n"
	"1:\tmovl %4,%%edi\n\t"
	"movl %%esi,%%eax\n\t"
	"movl %%edx,%%ecx\n\t"
	"repe\n\t"
	"cmpsb\n\t"
	"je 2f\n\t"		/* also works for empty string, see above */
	"xchgl %%eax,%%esi\n\t"
	"incl %%esi\n\t"
	"cmpb $0,-1(%%eax)\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"2:"
	:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
	:"cx","dx","di","si");
return __res;
}

/**
 * 字符长度获取
 */
extern inline int strlen(const char * s)
{
register int __res __asm__("cx");
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"		// al与es:edi比较
	"notl %0\n\t"	// 不想等就一直比较
	"decl %0"
	:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"di");
return __res;
}

extern char * ___strtok;	// 用于临时存放

/**
 * 利用字符串2中的字符，将字符串1根哥成token序列
 */
extern inline char * strtok(char * s,const char * ct)
{
register char * __res __asm__("si");
__asm__("testl %1,%1\n\t"
	"jne 1f\n\t"
	"testl %0,%0\n\t"
	"je 8f\n\t"
	"movl %0,%1\n"
	"1:\txorl %0,%0\n\t"
	"movl $-1,%%ecx\n\t"
	"xorl %%eax,%%eax\n\t"
	"cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"je 7f\n\t"			/* empty delimeter-string */
	"movl %%ecx,%%edx\n"
	"2:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 7f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 2b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 7f\n\t"
	"movl %1,%0\n"
	"3:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 5f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 3b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 5f\n\t"
	"movb $0,(%1)\n\t"
	"incl %1\n\t"
	"jmp 6f\n"
	"5:\txorl %1,%1\n"
	"6:\tcmpb $0,(%0)\n\t"
	"jne 7f\n\t"
	"xorl %0,%0\n"
	"7:\ttestl %0,%0\n\t"
	"jne 8f\n\t"
	"movl %0,%1\n"
	"8:"
	:"=b" (__res),"=S" (___strtok)
	:"0" (___strtok),"1" (s),"g" (ct)
	:"ax","cx","dx","di");
return __res;
}

/**
 * 内存块拷贝
 * @param dest 目标地址
 * @param src 来源地址
 * @param n	拷贝长度
 */
extern inline void * memcpy(void * dest,const void * src, int n)
{
__asm__("cld\n\t"		// 清方向位
	"rep\n\t"			// 重复
	"movsb"				// 拷贝指令，ds:esi->es:edx, esi++, edi++
	::"c" (n),"S" (src),"D" (dest)
	:"cx","si","di");
return dest;
}

/**
 * 内存块移动。 考虑移动方向
 * @param dest 目标地址
 * @param src 来源地址
 * @param n	复制字节数
 */
extern inline void * memmove(void * dest,const void * src, int n)
{
if (dest<src)
__asm__("cld\n\t"		// 清方向位
	"rep\n\t"
	"movsb"				// ds:esi->es:edi,esi++, edi++
	::"c" (n),"S" (src),"D" (dest)
	:"cx","si","di");
else
__asm__("std\n\t"		// 置方向位
	"rep\n\t"			// 从末端开始复制
	"movsb"				// ds:esi->es:edi, esi++, edi++
	::"c" (n),"S" (src+n-1),"D" (dest+n-1)
	:"cx","si","di");
return dest;
}

/**
 * 内存比较函数，遇到NULL页不停止比较
 * @param cs 内存块1
 * @param ct 内存块2
 * @param count 比较字节数
 */
extern inline int memcmp(const void * cs,const void * ct,int count)
{
register int __res __asm__("ax");
__asm__("cld\n\t"
	"repe\n\t"		// 相同则继续
	"cmpsb\n\t"		// 比较ds:esi, es:edi, esi++, edi++
	"je 1f\n\t"		// 相同，跳转到到1号标志
	"movl $1,%%eax\n\t"	// eax=1
	"jl 1f\n\t"			// 若内存块2的内容值小于内存块1的内容值， 跳转到标号1
	"negl %%eax\n"		// eax=-eax
	"1:"				// 结束标志
	:"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count)
	:"si","di","cx");
return __res;
}

/**
 * 在count字节大小的内存块中，寻找指定字符
 * @param cs 内存地址
 * @param c 字符
 * @param count 字节大小
 */
extern inline void * memchr(const void * cs,char c,int count)
{
register void * __res __asm__("di");
if (!count)
	return NULL;
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"		// al与es:edi做比较，edi++
	"je 1f\n\t"		// 想等则跳到1号标志
	"movl $1,%0\n"	// eax=1
	"1:\tdecl %0"	// eax--
	:"=D" (__res):"a" (c),"D" (cs),"c" (count)
	:"cx");
return __res;
}

/**
 * 内存置空
 * @param s 内存起始地址
 * @param c 重置为的字符串
 * @param count 字节数
 */
extern inline void * memset(void * s,char c,int count)
{
__asm__("cld\n\t"		// 清方方向位
	"rep\n\t"			// 重复
	"stosb"				// 将al放入es:edi, edi++
	::"a" (c),"D" (s),"c" (count)
	:"cx","di");
return s;
}

#endif
