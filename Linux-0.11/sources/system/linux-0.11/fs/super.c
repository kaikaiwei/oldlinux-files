/*
 *  linux/fs/super.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * super.c contains code to handle the super-block tables.
 * 超级块的操作
 */
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

#include <errno.h>
#include <sys/stat.h>

// 同步设备，高速缓冲区到磁盘上。
int sync_dev(int dev);
// 等待按键按下 kernel/chr_drv/tty_io.c
void wait_for_keypress(void);

/* set_bit uses setb, as gas doesn't recognize setc */
// set_bit使用setb指令，因为汇编起gas不能识别指令setc
/**
 * 测试指定为便宜出比特位的值（0或者1）， 并返回该比特位值（取名test_bit（）可能回更好
 * 嵌入式汇编宏。参数bitnr是比特位的偏移量，addr是测试比特位操作的起始地址
 * %0 ax（__res), %1 0, %2 bitnr, %3 addr.
 */
#define set_bit(bitnr,addr) ({ \
register int __res __asm__("ax"); \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

// 超级块结构数组， 共8项
struct super_block super_block[NR_SUPER];

/* this is initialized in init/main.c */
int ROOT_DEV = 0;	// 根设备号

/**
 * 锁定超级块
 */
static void lock_super(struct super_block * sb)
{
	cli();	// 关中断
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sb->s_lock = 1;	// 锁定标志
	sti();	// 开中断
}

/**
 * 释放超级块
 */
static void free_super(struct super_block * sb)
{
	cli();	// 关中断
	sb->s_lock = 0;	// 复位锁定标志
	wake_up(&(sb->s_wait));	// 唤醒等待该超级块的进程
	sti();	// 开中断
}

/**
 * 睡眠等待超级块
 */
static void wait_on_super(struct super_block * sb)
{
	cli();	// 关中断
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));	// 睡眠等待
	sti();	// 开中断
}

/**
 * 获取指定设备号的超级块
 * 返回该超级块结构指针
 */
struct super_block * get_super(int dev)
{
	struct super_block * s;

	// 设备号为0， 返回NULL
	if (!dev)
		return NULL;
	
	// 遍历超级块列表
	s = 0+super_block;
	while (s < NR_SUPER+super_block)
		// 设备号匹配的上
		if (s->s_dev == dev) {
			// 睡眠等待超级块
			wait_on_super(s);
			// 再次检查设备号
			if (s->s_dev == dev)
				return s;
			// 从头开始遍历
			s = 0+super_block; 
		} else
			s++;
	return NULL;
}

/**
 * 释放指定设备的超级块
 * 释放设备所使用的超级块数组项（置s_dev=0），并释放该设备i节点位图和逻辑块位图所占用的高速缓冲块
 * 如果超级块对应的文件系统是根文件系统，或者其i节点上已经在安装有其他的文件系统，则不能释放该超级块。
 */
void put_super(int dev)
{
	struct super_block * sb;
	struct m_inode * inode;
	int i;

	// 根文件系统，返回
	if (dev == ROOT_DEV) {
		printk("root diskette changed: prepare for armageddon\n\r");
		return;
	}

	// 获取超级块，没有对应的超级块，返回
	if (!(sb = get_super(dev)))
		return;

	// 如果该超级块指明本文件系统i节点上安装有其他的文件系统，则显示警告信息，返回
	if (sb->s_imount) {
		printk("Mounted disk changed - tssk, tssk\n\r");
		return;
	}

	// 锁定超级块
	lock_super(sb);
	// 释放设备所使用的超级块数组项
	sb->s_dev = 0;
	// I_MAP_SLOTS = 8， 释放该i节点位图
	for(i=0;i<I_MAP_SLOTS;i++)
		brelse(sb->s_imap[i]);
	// 逻辑块位图所在缓冲区占用的缓冲块
	for(i=0;i<Z_MAP_SLOTS;i++)
		brelse(sb->s_zmap[i]);
	// 释放超级块
	free_super(sb);
	return;
}

/**
 * 从设备上读取超级块到缓冲区中
 * 如果该设备超级块已经在高速缓冲中并且有效，则直接返回该超级块的指针
 */
static struct super_block * read_super(int dev)
{
	struct super_block * s;
	struct buffer_head * bh;
	int i,block;

	// 设备号不合法，没有指明设备
	if (!dev)
		return NULL;

	// 检查设备是否更换过盘片（软盘设备）， 如果更换过盘，则高速缓冲区有关该设备的所有缓冲块君失效
	// 需要进行失效处理（释放原来已经加载的文件系统）
	check_disk_change(dev);

	// 如果该设备的超级块已经在高速缓冲中，则直接返回该超级块的指针
	if (s = get_super(dev))
		return s;
	// 首先在超级块数据中找一个空项
	for (s = 0+super_block ;; s++) {
		if (s >= NR_SUPER+super_block)
			return NULL;
		if (!s->s_dev)
			break;
	}
	// 指定设备号
	s->s_dev = dev;
	// 对该超级块内存项进行部分初始化
	s->s_isup = NULL;
	s->s_imount = NULL;		// 无其他文件系统挂载在这里
	s->s_time = 0;			// 初始化时间
	s->s_rd_only = 0;		// 读写标志
	s->s_dirt = 0;			// 修改标志
	lock_super(s);		// 锁定超级块
	// 从设备上去读超级块到指向的缓冲区
	if (!(bh = bread(dev,1))) { // 如果缓冲区没有读到
		s->s_dev=0; // 清空超级块数组中的项
		free_super(s); // 释放超级块
		return NULL;
	}
	// 将设备上读取的超级块信息复制到超级块数组相应项结构中，并释放存放读取信息的高速缓冲区
	*((struct d_super_block *) s) =
		*((struct d_super_block *) bh->b_data);
	brelse(bh);

	// 如果读取到的超级块的文件系统魔术字段不对。数名设备上不是正确的文件系统。
	// 释放超级块，释放高速缓冲区
	if (s->s_magic != SUPER_MAGIC) {
		s->s_dev = 0;
		free_super(s);
		return NULL;
	}
	// i节点位图清空
	for (i=0;i<I_MAP_SLOTS;i++)
		s->s_imap[i] = NULL;
	// 逻辑块位图清空
	for (i=0;i<Z_MAP_SLOTS;i++)
		s->s_zmap[i] = NULL;
	// 逻辑块号，从第二块开始清理
	block=2;
	// 取设备上i节点位图
	for (i=0 ; i < s->s_imap_blocks ; i++)
		if (s->s_imap[i]=bread(dev,block))
			block++;
		else
			break;
	// 取设备上逻辑块位图
	for (i=0 ; i < s->s_zmap_blocks ; i++)
		if (s->s_zmap[i]=bread(dev,block))
			block++;
		else
			break;
	// 如果读取出的逻辑块数不等于位图应该占有的逻辑块数，说明文件系统文图信息有问题
	// 超级块初始化失败。释放申请的所有资源
	if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {
		for(i=0;i<I_MAP_SLOTS;i++)
			brelse(s->s_imap[i]);
		for(i=0;i<Z_MAP_SLOTS;i++)
			brelse(s->s_zmap[i]);
		s->s_dev=0;
		free_super(s);
		return NULL;
	}
	// 将位图的最低位设置为1，防止文件系统分配0号i节点。 如果设备上所有i节点已经被使用，查找函数回返回0
	s->s_imap[0]->b_data[0] |= 1;
	s->s_zmap[0]->b_data[0] |= 1;
	// 解锁该超级块，并返回超级块指针
	free_super(s);
	return s;
}

/**
 * 卸载文件系统的系统调用函数。 
 * @param dev_name 设备文件名
 */
int sys_umount(char * dev_name)
{
	struct m_inode * inode;
	struct super_block * sb;
	int dev;

	// 根据设备文件名找到对应的i节点，并取其中的设备号
	if (!(inode=namei(dev_name)))
		return -ENOENT;
	dev = inode->i_zone[0];

	// 如果不是块设备文件，则释放刚申请的i节点，返回出错号
	if (!S_ISBLK(inode->i_mode)) {
		iput(inode);
		return -ENOTBLK;
	}
	// 释放设备文件名的i节点
	iput(inode);
	// 如果是根文件系统，直接返回出错号
	if (dev==ROOT_DEV)
		return -EBUSY;

	// 取设备的超级块失败，或者该设备文件系统没有安装过。返回出错号
	if (!(sb=get_super(dev)) || !(sb->s_imount))
		return -ENOENT;
	// 如果超级块所指明的被安装的i节点，没有设备安装标记，显示警告信息
	if (!sb->s_imount->i_mount)
		printk("Mounted inode has i_mount=0\n");
	// 查找i节点表，查看是否有进程正在使用该设备上的文件，有则返回出错号
	for (inode=inode_table+0 ; inode<inode_table+NR_INODE ; inode++)
		if (inode->i_dev==dev && inode->i_count)
				return -EBUSY;
	// 安装标记设置为0，没有安装标记。
	sb->s_imount->i_mount=0;
	// 释放i节点位图
	iput(sb->s_imount);
	sb->s_imount = NULL;
	// 释放逻辑块位符
	iput(sb->s_isup);
	sb->s_isup = NULL;

	// 释放指定的超级块
	put_super(dev);
	// 同步高速缓冲区内容到设备
	sync_dev(dev);
	return 0;
}

/**
 * 文件系统挂载系统调用
 * @param dev_name 设备名称
 * @param dir_name 挂载点，安装到的目录名
 * @param rw_flag 读写标记
 */
int sys_mount(char * dev_name, char * dir_name, int rw_flag)
{
	struct m_inode * dev_i, * dir_i;
	struct super_block * sb;
	int dev;

	// // 根据设备文件名找到对应的i节点，并取其中的设备号
	if (!(dev_i=namei(dev_name)))
		return -ENOENT;
	dev = dev_i->i_zone[0];

	// 如果不是块设备
	if (!S_ISBLK(dev_i->i_mode)) {
		iput(dev_i); // 释放i节点
		return -EPERM;
	}
	iput(dev_i);	// 释放块设备

	// 根据挂载点目录，得到挂载点的i节点
	if (!(dir_i=namei(dir_name)))
		return -ENOENT;
	// 如果i节点被多次引用或者是根设备，释放i节点并返回错误号
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {
		iput(dir_i);
		return -EBUSY;
	}

	// 如果不是一个目录节点
	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -EPERM;
	}

	// 读取指定设备的超级块内容
	if (!(sb=read_super(dev))) {
		iput(dir_i);
		return -EBUSY;
	}
	// 如果超级块已经挂载，则返回错误
	if (sb->s_imount) {
		iput(dir_i);
		return -EBUSY;
	}
	// 如果挂载点已经挂载，则返回错误
	if (dir_i->i_mount) {
		iput(dir_i);
		return -EPERM;
	}

	// 挂载点
	sb->s_imount=dir_i;
	// 目录时挂载点
	dir_i->i_mount=1;
	// 节点已经修改标记
	dir_i->i_dirt=1;		/* NOTE! we don't iput(dir_i) */
	return 0;			/* we do that in umount */
}

/**
 * 挂载根文件系统
 * 在系统开机初始化设置时sys_setup调用的。kernel/blk_drv/hd.c
 */
void mount_root(void)
{
	int i,free;
	struct super_block * p;
	struct m_inode * mi;

	// 如果磁盘i节点结构不是32字节，则出错。 用于判断防止修改源代码时的不一致
	if (32 != sizeof (struct d_inode))
		panic("bad i-node size");

	// 初始化文件表数组（共64项， 也就是系统最多能同时打开64个文件），将所有文件结构中的引用计数设置为0
	for(i=0;i<NR_FILE;i++)
		file_table[i].f_count=0;

	// 如果根文件系统时软盘。提示：插入根文件系统盘，并按回车
	if (MAJOR(ROOT_DEV) == 2) {
		printk("Insert root floppy and press ENTER");
		wait_for_keypress();
	}

	// 初始化超级块数组，共8个
	for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++) {
		p->s_dev = 0;
		p->s_lock = 0;
		p->s_wait = NULL;
	}

	// 从设备读取超级块。读取失败则死机
	if (!(p=read_super(ROOT_DEV)))
		panic("Unable to mount root");

	// 获取设备上读取文件系统的根i节点，如果失败则显示出错信息
	if (!(mi=iget(ROOT_DEV,ROOT_INO)))
		panic("Unable to read root i-node");
	// 该节点的引用次数增加3次，因为下面也引用了该节点
	mi->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */

	// 设置改超级块的被安装文件系统i节点和被安装到的i节点
	p->s_isup = p->s_imount = mi;

	// 当前工作目录
	current->pwd = mi;
	// 根节点目录
	current->root = mi;

	// 统计设备上空闲块数。 首先令i等于超级块
	free=0;	
	i=p->s_nzones;
	// 根据逻辑块位图中相应比特位的占用情况统计空闲块数。
	// set_bit只是在厕所比特位，而非设置比特位。
	// i&8191 用于区的i节点号在当前块中的偏移值
	// i>>13 时将i除以8192， 除以一个磁盘块包含的比特位数
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
			free++;
	printk("%d/%d free blocks\n\r",free,p->s_nzones);

	// 统计设备上空闲i节点数。首先令i等于超级块中表明的设备上i节点总数+1。+1时将0节点统计进去
	free=0;
	i=p->s_ninodes+1;
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
			free++;
	printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}
