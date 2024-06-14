/*
 *  linux/fs/inode.c
 *
 *  i节点的函数，iget(), iput()和块映射函数bmap()， 
 *  主要是namei.c程序的路径名到i节点的映射函数namei() 
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/system.h>

// 内存节点表。 include/linux/fs.h， NR_INODE=32
struct m_inode inode_table[NR_INODE]={{0,},};

/**
 * 从设备中读取i节点 
 */
static void read_inode(struct m_inode * inode);
/**
 * 写入i节点到设备中
 */
static void write_inode(struct m_inode * inode);

/**
 * 等待指定的inode释放
 */
static inline void wait_on_inode(struct m_inode * inode)
{
	cli();		// 关中断
	while (inode->i_lock)
		sleep_on(&inode->i_wait);	// 在inode的wait队列上等待
	sti();		// 开中断
}

/**
 * 对指定的inode上锁
 */
static inline void lock_inode(struct m_inode * inode)
{
	cli();		// 关中断
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	inode->i_lock=1;	// 上锁
	sti();		// 开中断
}

/**
 * 解锁inode
 */
static inline void unlock_inode(struct m_inode * inode)
{
	inode->i_lock=0;
	wake_up(&inode->i_wait); // 唤醒等待的进程
}

/**
 * 设备无效（移除）导致释放所有的inode
 */
void invalidate_inodes(int dev)
{
	int i;
	struct m_inode * inode;

	// 遍历所有的inode_table
	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		// 等待解锁
		wait_on_inode(inode);
		if (inode->i_dev == dev) { // 设备号对上
			if (inode->i_count)	// inode 正在使用中
				printk("inode in use on removed disk\n\r");
			inode->i_dev = inode->i_dirt = 0; // 更新 设备号和已更新标记
		}
	}
}

/**
 * 同步所有inodes
 */
void sync_inodes(void)
{
	int i;
	struct m_inode * inode;

	// 遍历inode_table
	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);	// 等待解锁
		// 如果是已经更新标志，并且不是管道类型的，那么就写入磁盘
		if (inode->i_dirt && !inode->i_pipe)
			write_inode(inode);
	}
}

/**
 * 文件数据块映射到盘块的处理函数。
 * block位图处理函数。 bmap： block map
 * @param inode 文件i节点
 * @param block 文件的数据块号
 * @param create 创建标记
 * 如果创建标志置位，则在对应逻辑块不存在的时候申请新的磁盘块。
 * @return 返回设备上的逻辑块号（盘块号）
 */
static int _bmap(struct m_inode * inode,int block,int create)
{
	struct buffer_head * bh;
	int i;

	// 块号小于0
	if (block<0)
		panic("_bmap: block<0");
	// 如果块号大于 直接块数+ 间接块数 + 二次间接块数
	if (block >= 7+512+512*512)
		panic("_bmap: block>big");
	// 表示使用直接块
	if (block<7) {
		// 创建，并且i节点中对应块的逻辑块（区段）不为0
		if (create && !inode->i_zone[block])
			// 向相应设备申请一个磁盘块
			if (inode->i_zone[block]=new_block(inode->i_dev)) {
				inode->i_ctime=CURRENT_TIME; // 修改时间
				inode->i_dirt=1;			 // 更新标记
			}
		// 返回对应的磁盘块
		return inode->i_zone[block];
	}
	block -= 7; // 一次间接
	if (block<512) { 
		// 创建标志，并且inode->i_zone[7]为0，表示要创新新的block
		if (create && !inode->i_zone[7])
			if (inode->i_zone[7]=new_block(inode->i_dev)) {
				inode->i_dirt=1;	// 更新标记
				inode->i_ctime=CURRENT_TIME;	// 时间
			}
		// 如果间接块为0， 直接返回NULL
		if (!inode->i_zone[7])
			return 0;
		// 读取一次间接块信息上来，设备号，块号，如果没有高度缓存区，则返回NULL
		if (!(bh = bread(inode->i_dev,inode->i_zone[7])))
			return 0;
		// 取该间接块上第bloc项中的逻辑块号
		i = ((unsigned short *) (bh->b_data))[block];
		// 创建标志，并且块号不为0
		if (create && !i)
			// 新建块
			if (i=new_block(inode->i_dev)) {
				((unsigned short *) (bh->b_data))[block]=i;
				bh->b_dirt=1; // 块更新标记
			}
		// 释放块
		brelse(bh); 
		return i;
	}
	block -= 512; // 减去512，使用二次间接块
	// 创建标记， 二次间接块还没有创建
	if (create && !inode->i_zone[8])
		// 新建块
		if (inode->i_zone[8]=new_block(inode->i_dev)) {
			inode->i_dirt=1;	// 修改标记
			inode->i_ctime=CURRENT_TIME;	// 创建时间修改
		}
	// 如果二次间接块创建失败，返回NULL
	if (!inode->i_zone[8])
		return 0;
	// 读取缓冲区头信息，如果没有读到，返回NULL
	if (!(bh=bread(inode->i_dev,inode->i_zone[8])))
		return 0;
	// 取该二次间接块的一级块上第block/512项中的逻辑块号
	i = ((unsigned short *)bh->b_data)[block>>9];
	// 如果是创建，并且二次间接块的逻辑块号为0， 创建新的块
	if (create && !i)
		// 创建新的块
		if (i=new_block(inode->i_dev)) {
			((unsigned short *) (bh->b_data))[block>>9]=i; // 更新二次间接块多硬的逻辑块号
			bh->b_dirt=1;	// 缓冲区头更新
		}
	// 释放缓冲区头
	brelse(bh);
	// 如果逻辑块号为0，表示申请磁盘块失败
	if (!i)
		return 0;
	// 读取该块的缓冲区头（二级块）
	if (!(bh=bread(inode->i_dev,i)))
		return 0;
	// 取该二级块的逻辑块号。511是为了不超过512
	i = ((unsigned short *)bh->b_data)[block&511];
	// 如果是创建，并且逻辑块号为0
	if (create && !i)
		// 申请新的块号。 
		if (i=new_block(inode->i_dev)) {
			((unsigned short *) (bh->b_data))[block&511]=i;
			bh->b_dirt=1; // 缓冲区头已经修改
		}
	// 释放该缓冲区
	brelse(bh);
	return i;
}

/**
 * 根据i节点信息取文件数据块block在设备上对应的逻辑块号
 */
int bmap(struct m_inode * inode,int block)
{
	return _bmap(inode,block,0); // 不创建
}

/**
 * 创建文件数据块block在设备上对应的逻辑块，并返回设备上对应的逻辑块号
 */
int create_block(struct m_inode * inode, int block)
{
	return _bmap(inode,block,1);
}
		
/**
 * 释放一个i节点。 回写设备 
 */
void iput(struct m_inode * inode)
{
	// 检查参数
	if (!inode)
		return;
	// 对inode解锁，如果已经上锁的话
	wait_on_inode(inode);
	// 如果i_count=0， 表明没有在使用中
	if (!inode->i_count)
		panic("iput: trying to free free inode");
	// 如果是管道节点
	if (inode->i_pipe) {
		wake_up(&inode->i_wait); // 唤醒等待该管道的进程
		// 引用次数减1. 如果还有引用就返回
		if (--inode->i_count)
			return;
		// 释放管道占用的内存页面
		free_page(inode->i_size);
		// 复位该节点的引用计数制
		inode->i_count=0;
		// 复位已修改标志
		inode->i_dirt=0;
		// 复位管道标志
		inode->i_pipe=0;
		return;
	}

	// 若i节点对应设备号=0， 则将此节点引用计数递减1，返回
	if (!inode->i_dev) {
		inode->i_count--;
		return;
	}

	// 如果是块设备的i节点，此时逻辑块字段0中是设备号，则刷新该设备，并等待i节点解锁
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		wait_on_inode(inode);
	}
repeat:
	// 如果还有其他引用，则返回
	if (inode->i_count>1) {
		inode->i_count--;
		return;
	}

	// 如果i节点的链接数为0， 释放该i节点的所有逻辑块，并释放该i节点
	if (!inode->i_nlinks) {
		truncate(inode);	// 释放i节点的所有逻辑块。 linux/fs/trancate.c
		free_inode(inode); // 释放i节点
		return;
	}
	// 如果i节点的已经修改，更新该i节点
	if (inode->i_dirt) {
		write_inode(inode);	/* we can sleep - so do again */ // 同步该i节点到磁盘。 本文后面
		wait_on_inode(inode); // 等待i节点解锁
		goto repeat;
	}
	inode->i_count--; // 减少引用计数
	return;
}

/**
 * 从i节点表（inode_table）中获取一个空闲i节点项
 * 寻找引用计数为0的i节点，并将其写盘后清0，返回其指针
 */
struct m_inode * get_empty_inode(void)
{
	struct m_inode * inode;
	static struct m_inode * last_inode = inode_table; // last_inode指向i节点表的第一项
	int i;

	do {
		inode = NULL;
		// 遍历inode_table
		for (i = NR_INODE; i ; i--) {
			// 如果last_inode已经指向最后一项
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table; // 指向第一个
			// 如果当前项的引用计数为0
			if (!last_inode->i_count) {
				inode = last_inode; // 找到了该inode
				// 没有已修改标记，没有锁定标记
				if (!inode->i_dirt && !inode->i_lock)
					break;
			}
		}
		
		// 如果inode为空，意味着没有找到。 死机
		if (!inode) {
			for (i=0 ; i<NR_INODE ; i++)
				printk("%04x: %6d\t",inode_table[i].i_dev,
					inode_table[i].i_num);
			panic("No free inodes in mem");
		}
		
		// 等待inode解锁
		wait_on_inode(inode);

		// 如果inode是脏的，那么就写盘。
		while (inode->i_dirt) {
			write_inode(inode);
			wait_on_inode(inode); // 等待inode释放
		}
	} while (inode->i_count); // 只要找到的inode计数大于0
	
	// inode对应的内存清0
	memset(inode,0,sizeof(*inode));
	// 设置引用计数
	inode->i_count = 1;
	return inode;
}

/**
 * 获取管道inode
 */
struct m_inode * get_pipe_inode(void)
{
	struct m_inode * inode;

	// 获取空白的inode，没有获取到则返回
	if (!(inode = get_empty_inode()))
		return NULL;
	// 获取空闲页面。如果没有空闲页面，释放inode。
	if (!(inode->i_size=get_free_page())) {
		inode->i_count = 0;
		return NULL;
	}
	// 增加引用计数。
	inode->i_count = 2;	/* sum of readers/writers */
	// 复位管道头指针， fs.h中
	PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;
	// 管道标记
	inode->i_pipe = 1;
	return inode;
}

/**
 * 从设备上读取指定节点号的i节点
 * @param dev 设备号
 * @param nr i节点号
 */
struct m_inode * iget(int dev,int nr)
{
	struct m_inode * inode, * empty;

	// 检查设备号
	if (!dev)
		panic("iget with dev==0");
	// 获取空闲i节点
	empty = get_empty_inode();
	// 遍历i节点
	inode = inode_table;
	while (inode < NR_INODE+inode_table) {
		// 不是指定设备，节点号不等于指定节点号， 跳过
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode++;
			continue;
		}
		// 等待inode解锁
		wait_on_inode(inode);
		// 再次检查设备号和节点号
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode = inode_table;
			continue;
		}

		// 增加引用计数
		inode->i_count++;
		// 如果该i节点是其他文件系统的安装点
		if (inode->i_mount) {
			int i;
			
			// 载爱超级块表中搜寻按转在此i节点的超级快
			for (i = 0 ; i<NR_SUPER ; i++)
				if (super_block[i].s_imount==inode)
					break;
			// 如果没有找到超级快，显示出错信息。
			if (i >= NR_SUPER) {
				printk("Mounted inode hasn't got sb\n");
				// 释放该i节点
				if (empty)
					iput(empty);
				return inode; // 返回该i节点的指针
			}
			// 将该i节点写盘。
			iput(inode);
			// 从安装在此的文件系统的超级块中获得设备号
			dev = super_block[i].s_dev;
			nr = ROOT_INO;
			inode = inode_table; // inode指向inode_table表头，表示需要重新遍历
			continue;
		}
		// 如果empty优质，则释放
		if (empty)
			iput(empty);
		return inode;
	}
	// 如果没有找到空闲inode，则返回
	if (!empty)
		return (NULL);
	// 初始化inode
	inode=empty;
	inode->i_dev = dev;	// 设备号
	inode->i_num = nr;	// inode号
	read_inode(inode);	// 从磁盘上读取inode
	return inode;
}

/**
 * 从设备中读取指定i节点的信息到内存中（缓冲块中）
 * @param inode
 */
static void read_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	// 对inode上锁
	lock_inode(inode);
	// 获取超级块，如果超级块为空，则系统出错
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to read inode without dev");
	// 该节点的逻辑块号= （启动块+超级块） + i节点位图所占用的块数 + 逻辑块位图占用的块数 + （i节点号-1）/每块含有的i节点数
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	// 从设备上读取该i节点所在的逻辑块，并复制其中指定i节点的内容
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	// 复制其中指定i节点的内容
	*(struct d_inode *)inode =
		((struct d_inode *)bh->b_data)
			[(inode->i_num-1)%INODES_PER_BLOCK];
	// 释放缓冲区
	brelse(bh);
	// 解锁该i节点
	unlock_inode(inode);
}

/**
 * 向设备写入指定inode
 * 
 */
static void write_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	// 锁定inode
	lock_inode(inode);
	// 如果没有更新标志 或设备号为0， 解锁该inode，并返回
	if (!inode->i_dirt || !inode->i_dev) {
		unlock_inode(inode);
		return;
	}

	// 获取超级块信息
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to write inode without device");

	// 计算逻辑块号
	// 该节点的逻辑块号= （启动块+超级块） + i节点位图所占用的块数 + 逻辑块位图占用的块数 + （i节点号-1）/每块含有的i节点数
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	
	// 向设备上写入该i节点所在的逻辑块，复制i节点内容道设备缓冲区上
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	((struct d_inode *)bh->b_data)
		[(inode->i_num-1)%INODES_PER_BLOCK] =
			*(struct d_inode *)inode;
	bh->b_dirt=1; 	 // 设置更新标记
	inode->i_dirt=0; // 清inode的修改标记
	brelse(bh);		 // 释放缓冲区
	unlock_inode(inode);	// 解锁inode
}
