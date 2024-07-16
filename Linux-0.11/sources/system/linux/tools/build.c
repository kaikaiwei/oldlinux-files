/*
 *  linux/tools/build.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This file builds a disk-image from three different files:
 *
 * - bootsect: max 510 bytes of 8086 machine code, loads the rest
 * - setup: max 4 sectors of 8086 machine code, sets up system parm
 * - system: 80386 code for actual system
 *
 * It does some checking that all files are of the correct type, and
 * just writes the result to stdout, removing headers and padding to
 * the right amount. It also writes some system data to stderr.

 * 该程序从三个不同的程序中创建磁盘映像文件
 * bootsect： 该文件的8086机器码最长510字节，用于加载其他程序
 * setup：该文件的8086机器码最长为4个扇区，用于设置系统参数
 * system： 实际系统的80386代码
 * 
 * 该程序首先检查所有程序模块的类型是否正确，并将检查结果在终端上显示出来
 * 然后删除模块头部并扩充到正确的长度。该程序也会将一些系统数据写到stderr。
 */

#include <stdio.h>	/* fprintf */
#include <stdlib.h>	/* contains exit */
#include <sys/types.h>	/* unistd.h needs this */
#include <unistd.h>	/* contains read/write */
#include <fcntl.h>

// MINIX 二进制模块头部长度为32B
#define MINIX_HEADER 32
// GCC头部信息长度为1024B
#define GCC_HEADER 1024

// system文件最长字节数， SYS_SIZE * 16 = 128KB
#define SYS_SIZE 0x2000

/* max nr of sectors of setup: don't change unless you also change
 * bootsect etc */
// setup最大长度为4个扇区
#define SETUP_SECTS 4

// 用于出错时显示语句中表示扇区数
#define STRINGIFY(x) #x

// 显示出错信息
void die(char * str)
{
	fprintf(stderr,"%s\n",str);
	exit(1);
}

// 显示程序使用方法并退出
void usage(void)
{
	die("Usage: build bootsect setup system [> image]");
}

/**
 * 
 */
int main(int argc, char ** argv)
{
	int i,c,id;
	char buf[1024];

	// 如果命令行参数不是4个。 显示用法并退出
	if (argc != 4)
		usage();
	// 初始化buffer信息，全部设置为0
	for (i=0;i<sizeof buf; i++) buf[i]=0;
	// 只读方式打开bootsect
	if ((id=open(argv[1],O_RDONLY,0))<0)
		die("Unable to open 'boot'");
	// 读取MINIX执行头部信息。
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)
		die("Unable to read header of 'boot'");
	// 0x3001 minix头部amagic魔数， 0x10， a_flag可执行； 0x04 a_cpu， Intel 8086机器码
	if (((long *) buf)[0]!=0x04100301)
		die("Non-Minix header of 'boot'");
	// 判断头部长度字段a_hdrlen字节是否正确。
	if (((long *) buf)[1]!=MINIX_HEADER)
		die("Non-Minix header of 'boot'");
	// 判断数据段长a_bss字段内容是否为0
	if (((long *) buf)[3]!=0)
		die("Illegal data segment in 'boot'");
	// 判断堆a_bss字段是否为0
	if (((long *) buf)[4]!=0)
		die("Illegal bss in 'boot'");
	// 判断执行点a_entry字段是否为0
	if (((long *) buf)[5] != 0)
		die("Non-Minix header of 'boot'");
	// 判断符号表长字段a_sym 的内容是否为0
	if (((long *) buf)[7] != 0)
		die("Illegal symbol table in 'boot'");
	// 读取实际代码数据，应该返回读取字节数512字节
	i=read(id,buf,sizeof buf);
	fprintf(stderr,"Boot sector %d bytes.\n",i);
	if (i != 512)
		die("Boot block must be exactly 512 bytes");
	// 判断boot块0x510处是否有可引导标志0xAA55
	if ((*(unsigned short *)(buf+510)) != 0xAA55)
		die("Boot block hasn't got boot flag (0xAA55)");
	// 将boot块512字节数据写入到标准输出stdout，若写出字节数不对，则退出
	i=write(1,buf,512);
	if (i!=512)
		die("Write call failed");
	close (id);
	
	// 开始处理setup模块。 首先以只读方式打开该模块。 若出错则显示出错信息，退出
	if ((id=open(argv[2],O_RDONLY,0))<0)
		die("Unable to open 'setup'");
	// 读取该文件中minix执行头部信息，32字节。 
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)
		die("Unable to read header of 'setup'");
	// 0x0301 minix头部a_magic魔数； 0x10 a_flag 可执行； 0x04 a_cpu， Intenl 8086机器码
	if (((long *) buf)[0]!=0x04100301)
		die("Non-Minix header of 'setup'");
	// 判断头部长度字段a_hdrlen字节是否正确
	if (((long *) buf)[1]!=MINIX_HEADER)
		die("Non-Minix header of 'setup'");
	// 判断数据段长度a_data字段内容是否为0
	if (((long *) buf)[3]!=0)
		die("Illegal data segment in 'setup'");
	// 判断堆a_bss字段内容是否为0
	if (((long *) buf)[4]!=0)
		die("Illegal bss in 'setup'");
	// 判断执行点a_entry字段内容是否为0
	if (((long *) buf)[5] != 0)
		die("Non-Minix header of 'setup'");
	// 判断符号表长度字段a_sym的额你容是否为0
	if (((long *) buf)[7] != 0)
		die("Illegal symbol table in 'setup'");
	// 读取可执行代码数据， 并写到标准输出stdout
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c )
		if (write(1,buf,c)!=c)
			die("Write call failed");
	close (id); // 关闭setup模块

	// 若setup模块长度大于4个扇区，则出错
	if (i > SETUP_SECTS*512)
		die("Setup exceeds " STRINGIFY(SETUP_SECTS)
			" sectors - rewrite build/boot/setup");
	fprintf(stderr,"Setup is %d bytes.\n",i);

	// 将船冲去buf清0
	for (c=0 ; c<sizeof(buf) ; c++)
		buf[c] = '\0';
	// 若setup长度小于4*512字节，则用\0将setup填足4*512字节
	while (i<SETUP_SECTS*512) {
		c = SETUP_SECTS*512-i;
		if (c > sizeof(buf))
			c = sizeof(buf);
		if (write(1,buf,c) != c)
			die("Write call failed");
		i += c;
	}
	
	// 处理system模块。首先以只读方式打开该文件
	if ((id=open(argv[3],O_RDONLY,0))<0)
		die("Unable to open 'system'");
	// system模块是gcc格式的文件， 先读取gcc格式的头部结构信息
	if (read(id,buf,GCC_HEADER) != GCC_HEADER)
		die("Unable to read header of 'system'");
	// 该结构中的执行代码入口点字段a_entry值应该为0
	if (((long *) buf)[5] != 0)
		die("Non-GCC header of 'system'");
	// 读取随后的执行代码数据，并写到标准输出stdout。
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c )
		if (write(1,buf,c)!=c)
			die("Write call failed");
	close(id); // 关闭文件
	fprintf(stderr,"System is %d bytes.\n",i);
	// 若system代码数据段长度超过SYS_SIZE或128KB， 则显示出错信息，退出。
	if (i > SYS_SIZE*16)
		die("System is too big");
	return(0);
}
