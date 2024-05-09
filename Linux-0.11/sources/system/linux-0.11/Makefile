#
# if you want the ram-disk device, define this to be the
# size in blocks.
#  如果你想用ram盘设备的话，就需要定义块的大小
RAMDISK = #-DRAMDISK=512

AS86	=as86 -0 -a  # 8086 汇编编译器和连接器
LD86	=ld86 -0	 # -0 生成目标8086目标程序，-a 生成于gas和gad部分兼容的代码

AS	=gas # gnu汇编编译器。后更名为as
LD	=gld # gnu连接器。后更名为ld
# gnu编译器的运行时参数。
# -s： 输出文件中省略的所有符号信息。
# -x删除所有局部符号。
# -M 在标准输出设备上打印链接映像。
LDFLAGS	=-s -x -M 
# gcc是gnu的编译器
CC	=gcc $(RAMDISK)
# gcc 的参数。 
# -Wall 打印所有的警告信息
# -0 优化级别
# -fstrength-reduce 优化循环语句
# -fomit-freme-pointer：表示在函数调用时不需要存储堆栈帧指针。此选项可减小生成的代码镜像大小， 一般在启用 -On 优化后会自动打开，也有的平台是默认打开的。
# -fcombine-regs：表示将堆栈帧指针存储在寄存器中，帧指针是用来指示当前函数的栈帧（stack frame）的指针，在调试时可以帮助跟踪函数调用的堆栈信息。在 AArch32 架构中，堆栈帧指针存储在寄存器 R11（A32代码）或寄存器 R7（T32代码）中；在AArch64架构中，堆栈帧指针存储在寄存器 X29 中。作为帧指针使用的寄存器不能用作通用寄存器，但如果使用 -fomit-frame-pointer 选项编译，则可用作通用寄存器。
# -mbstring-insns是linus自己为gcc增加的选项，用于字符串指令优化程序，可以去掉。
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer \
-fcombine-regs -mstring-insns
# cpp是gcc的预处理程序。
# -nostdinc 不要搜索标砖目录中的头问文件，
# -I 指定搜索头文件的路径为include
CPP	=cpp -nostdinc -Iinclude

#
# ROOT_DEV specifies the default root-device when making the image.
# This can be either FLOPPY, /dev/xxxx or empty, in which case the
# default of /dev/hd6 is used by 'build'.
#  穿进啊内核时映像文件时所使用的文件系统和系统所在的设备。
ROOT_DEV=/dev/hd6

# kernel mm和fs 生成的目标代码文件。
# 也就是说，这里就是所有的中间文件。
ARCHIVES=kernel/kernel.o mm/mm.o fs/fs.o
DRIVERS =kernel/blk_drv/blk_drv.a kernel/chr_drv/chr_drv.a
MATH	=kernel/math/math.a
LIBS	=lib/lib.a

# 老式的隐式规则。表示使用下面命令将所有的.c文件转换为.s文件
# $< 表示第一个先决条件
# $*.s 自动目标文件
# -S 汇编
# -o 目标文件
.c.s:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -c -o $*.o $<

# 顶级不表
all:	Image

# Image目标由boot/bootsect boot/setup tools/system tools/build四个组成。
# 接下来说构建命令。由tools/build工具将boot/bootsect boot/setup tools/system以 $(ROOT_DEV)为根文件系统组成内核镜像Image。
# sync命令。将缓冲块中内容强制更新到磁盘上
Image: boot/bootsect boot/setup tools/system tools/build
	tools/build boot/bootsect boot/setup tools/system $(ROOT_DEV) > Image
	sync

# disk这个目标由Image生成。
# dd命令：bs表示一次传输多少内容。if表示输入设备，of表示输出设备
# /dev/PS0是软盘文件，表示输出设备
disk: Image
	dd bs=8192 if=Image of=/dev/PS0

# 构建目标来依赖于tools/build.c
# gcc -o [target] src
tools/build: tools/build.c
	$(CC) $(CFLAGS) \
	-o tools/build tools/build.c

# head.o 来自于header.s
boot/head.o: boot/head.s

# tools/system来自 head.o和main.o 还有这个系统的依赖。
# 然后是链接器
# 最后输出到tools/system。 
# 中间的输出就是所谓的system.map，也就是内核映射
tools/system:	boot/head.o init/main.o \
		$(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) boot/head.o init/main.o \
	$(ARCHIVES) \
	$(DRIVERS) \
	$(MATH) \
	$(LIBS) \
	-o tools/system > System.map

kernel/math/math.a:
	(cd kernel/math; make)

kernel/blk_drv/blk_drv.a:
	(cd kernel/blk_drv; make)

kernel/chr_drv/chr_drv.a:
	(cd kernel/chr_drv; make)

kernel/kernel.o:
	(cd kernel; make)

mm/mm.o:
	(cd mm; make)

fs/fs.o:
	(cd fs; make)

lib/lib.a:
	(cd lib; make)

boot/setup: boot/setup.s
	$(AS86) -o boot/setup.o boot/setup.s
	$(LD86) -s -o boot/setup boot/setup.o

boot/bootsect:	boot/bootsect.s
	$(AS86) -o boot/bootsect.o boot/bootsect.s
	$(LD86) -s -o boot/bootsect boot/bootsect.o

tmp.s:	boot/bootsect.s tools/system
	(echo -n "SYSSIZE = (";ls -l tools/system | grep system \
		| cut -c25-31 | tr '\012' ' '; echo "+ 15 ) / 16") > tmp.s
	cat boot/bootsect.s >> tmp.s

clean:
	rm -f Image System.map tmp_make core boot/bootsect boot/setup
	rm -f init/*.o tools/system tools/build boot/*.o
	(cd mm;make clean)
	(cd fs;make clean)
	(cd kernel;make clean)
	(cd lib;make clean)

backup: clean
	(cd .. ; tar cf - linux | compress - > backup.Z)
	sync

# sed为，在本文件155行的后面开始，输出到tmp_make中。 对init目录下的所有.c文件（main.c）执行预处理操作。
# 这里的-M表示告诉于处理器，这里的规则。
dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile
	(cd fs; make dep)
	(cd kernel; make dep)
	(cd mm; make dep)

### Dependencies:
init/main.o : init/main.c include/unistd.h include/sys/stat.h \
  include/sys/types.h include/sys/times.h include/sys/utsname.h \
  include/utime.h include/time.h include/linux/tty.h include/termios.h \
  include/linux/sched.h include/linux/head.h include/linux/fs.h \
  include/linux/mm.h include/signal.h include/asm/system.h include/asm/io.h \
  include/stddef.h include/stdarg.h include/fcntl.h 
