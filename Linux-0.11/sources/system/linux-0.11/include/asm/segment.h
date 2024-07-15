// Intel CPU中段寄存器或与段寄存器有关的内存操作函数
/**
 * 读取fs段中指定地址处的字节
 * @param addr 指定的内存地址
 * @return 返回的字节
 */
extern inline unsigned char get_fs_byte(const char * addr)
{
	unsigned register char _v;

	__asm__ ("movb %%fs:%1,%0":"=r" (_v):"m" (*addr));
	return _v;
}

/**
 * 读取fs段中指定地址处的字
 * @param addr 指定的内存地址
 * @return 返回的字节
 */
extern inline unsigned short get_fs_word(const unsigned short *addr)
{
	unsigned short _v;

	__asm__ ("movw %%fs:%1,%0":"=r" (_v):"m" (*addr));
	return _v;
}

/**
 * 读取fs段中指定地址处的长字， 4字节
 * @param addr 指定的内存地址
 * @return 返回的字节
 */
extern inline unsigned long get_fs_long(const unsigned long *addr)
{
	unsigned long _v;

	__asm__ ("movl %%fs:%1,%0":"=r" (_v):"m" (*addr)); \
	return _v;
}

/**
 * 在fs段中指定地址处写入字节
 * @param val  写入的字节
 * @param addr 指定的内存地址
 */
extern inline void put_fs_byte(char val,char *addr)
{
__asm__ ("movb %0,%%fs:%1"::"r" (val),"m" (*addr));
}

/**
 * 在fs段中指定地址处写入字
 * @param val  写入的字
 * @param addr 指定的内存地址
 */
extern inline void put_fs_word(short val,short * addr)
{
__asm__ ("movw %0,%%fs:%1"::"r" (val),"m" (*addr));
}

/**
 * 在fs段中指定地址处写入长字
 * @param val  写入的长字
 * @param addr 指定的内存地址
 */
extern inline void put_fs_long(unsigned long val,unsigned long * addr)
{
__asm__ ("movl %0,%%fs:%1"::"r" (val),"m" (*addr));
}

/*
 * Someone who knows GNU asm better than I should double check the followig.
 * It seems to work, but I don't know if I'm doing something subtly wrong.
 * --- TYT, 11/24/91
 * [ nothing wrong here, Linus ]
 */

/**
 * 取fs段寄存器的值并返回
 */
extern inline unsigned long get_fs() 
{
	unsigned short _v;
	__asm__("mov %%fs,%%ax":"=a" (_v):);
	return _v;
}

/**
 * 取fs段寄存器的值并返回
 */
extern inline unsigned long get_ds() 
{
	unsigned short _v;
	__asm__("mov %%ds,%%ax":"=a" (_v):);
	return _v;
}

/**
 * 设置fs段寄存器的值
 */
extern inline void set_fs(unsigned long val)
{
	__asm__("mov %0,%%fs"::"a" ((unsigned short) val));
}

