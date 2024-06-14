/*
 *  linux/fs/bitmap.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */
/**
 * 针对i节点位图和逻辑块位图进行释放和占用处理。
 * 操作i节点的函数时free_inode和new_inode。
 * 操作逻辑块位图的是free_block和new_block
 */
#include <string.h>

#include <linux/sched.h>	// 调度头文件
#include <linux/kernel.h>	// 内核头文件

// 将对应addr的缓冲区，填入0
// eax = 0， ecx=256， edx=addr
#define clear_block(addr) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"stosl" \
	::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)):"cx","di")

// 置位指定地址开始的nr个偏移量的比特位， nr可以大于32. 
// 返回原来的比特位（1或者0）
// eax=0， %2=nr 偏移量， %3 addr。
#define set_bit(nr,addr) ({\
register int res __asm__("ax"); \
__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

// 复位指定地址开始的nr个偏移量的比特位，nr可以大于32
// 返回原比特位的反码（1或者0）
#define clear_bit(nr,addr) ({\
register int res __asm__("ax"); \
__asm__ __volatile__("btrl %2,%3\n\tsetnb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

// 从addr开始寻找第一个0值比特位
// %0 eax（返回值）， %1， eax=0， %2 esi（addr）
#define find_first_zero(addr) ({ \
int __res; \
__asm__("cld\n" \
	"1:\tlodsl\n\t" \
	"notl %%eax\n\t" \
	"bsfl %%eax,%%edx\n\t" \
	"je 2f\n\t" \
	"addl %%edx,%%ecx\n\t" \
	"jmp 3f\n" \
	"2:\taddl $32,%%ecx\n\t" \
	"cmpl $8192,%%ecx\n\t" \
	"jl 1b\n" \
	"3:" \
	:"=c" (__res):"c" (0),"S" (addr):"ax","dx","si"); \
__res;})

/**
 * 释放设备dev上数据区的逻辑块block
 * @param dev 设备号
 * @param block 块号
 * 复位指定逻辑块block的逻辑位图比特位
 */
void free_block(int dev, int block)
{
	struct super_block * sb;
	struct buffer_head * bh;

	// 获得指定设备的超级快，fs.h中，在fs/super.c中。
	if (!(sb = get_super(dev)))
		panic("trying to free block on nonexistent device");
	// 如果逻辑块号小于超级快的第一个逻辑块号，或者大于设备上行总逻辑块数
	if (block < sb->s_firstdatazone || block >= sb->s_nzones)
		panic("trying to free block not in datazone");
	// 从hash表中寻找该块数据。 找到了要判断有效性
	bh = get_hash_table(dev,block);
	if (bh) {
		// count 不为1，表示有其他地方在用
		if (bh->b_count != 1) {
			printk("trying to free block (%04x:%d), count=%d\n",
				dev,block,bh->b_count);
			return;
		}
		// 清除修改标志和更新标志，并释放
		bh->b_dirt=0;
		bh->b_uptodate=0;
		brelse(bh);
	}
	// 计算block在数据区开始算起的数据逻辑块号（从1开始计数）
	block -= sb->s_firstdatazone - 1 ;
	// 复位逻辑块位图对应的比特位。 若对应比特位原来是0，则出错
	if (clear_bit(block&8191,sb->s_zmap[block/8192]->b_data)) {
		printk("block (%04x:%d) ",dev,block+sb->s_firstdatazone-1);
		panic("free_block: bit already cleared");
	}
	// 置相应的逻辑块位图所在缓冲区已修改标志
	sb->s_zmap[block/8192]->b_dirt = 1;
}

/**
 * 向设备dev申请一个逻辑块（盘块，区块）， 返回逻辑块号（盘块号）
 * 置位指定逻辑块block的逻辑块比特位图
 */
int new_block(int dev)
{
	struct buffer_head * bh;
	struct super_block * sb;
	int i,j;

	// 获取dev的超级块
	if (!(sb = get_super(dev)))
		panic("trying to get new block from nonexistant device");
	// 扫描逻辑块位图，寻找第一个0比特位。获取该逻辑块的块号。
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if (bh=sb->s_zmap[i])
			if ((j=find_first_zero(bh->b_data))<8192)
				break;
	// 如果扫面完还是没有找到，或者位图所在的缓冲块无效，则返回0， 退出。
	if (i>=8 || !bh || j>=8192)
		return 0;
	// 置位新逻辑块对应的逻辑块位图中的比特位，如果已经置位，则死机
	if (set_bit(j,bh->b_data))
		panic("new_block: bit already set");
	// 置位已修改标志。
	bh->b_dirt = 1;
	// 如果新的逻辑块大于设备上的逻辑块总数，说明逻辑块在设备上不存在，申请失败，返回0
	j += i*8192 + sb->s_firstdatazone-1;
	if (j >= sb->s_nzones)
		return 0;
	// 读取设备上该新逻辑块的数据。 如果失败则死机
	if (!(bh=getblk(dev,j)))
		panic("new_block: cannot get block");
	// 如果引用计数不为1，则死机
	if (bh->b_count != 1)
		panic("new block: count is != 1");
	// 清理block对应高速缓冲区内的数据
	clear_block(bh->b_data);
	bh->b_uptodate = 1; // 更新标志
	bh->b_dirt = 1;		// 已经修改标志
	brelse(bh);			// 释放块
	return j;			// 返回块号
}

/**
 * 释放指定的i节点。复位对应i节点位图比特位
 * @param inode i节点
 */
void free_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;

	// i节点为空，返回
	if (!inode)
		return;

	// i节点对应的设备号为0，说明该接i节点无用。则用0清空对应i节点所占用的内存区，并返回。
	if (!inode->i_dev) {
		memset(inode,0,sizeof(*inode));
		return;
	}

	// 如果该节点还有其他程序在使用，说明该节点无用，说明内核有问题，死机
	if (inode->i_count>1) {
		printk("trying to free inode with count=%d\n",inode->i_count);
		panic("free_inode");
	}

	// 如果文件目录项连接数不为0，则表示还有其他文件目录项使用该接i节点。不应释放，而应该返回等。
	if (inode->i_nlinks)
		panic("trying to free inode with links");

	// 获取i节点设备号对应的超级快
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to free inode on nonexistent device");

	// 如果该节点号=0或者大于该设备上所有i节点总数，则出错
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)
		panic("trying to free inode 0 or nonexistant inode");

	// 如果该i节点对应的节点位图不存在，则出错
	if (!(bh=sb->s_imap[inode->i_num>>13]))
		panic("nonexistent imap in superblock");

	// 复位i节点对应的节点位图中的比特位。如果该比特位已经等于0，则出错
	if (clear_bit(inode->i_num&8191,bh->b_data))
		printk("free_inode: bit already cleared.\n\r");
	
	// 置i节点位图所在缓冲区1已修改标志。并清空该接i节点结构所占内存区
	bh->b_dirt = 1;
	memset(inode,0,sizeof(*inode));
}

/**
 * 为设备dev建立一个新i节点。返回该i节点的指针
 * 在内存i节点表中获取一个空闲i节点表项，并从i节点位图中找一个空闲i节点
 * @param dev 设备号
 */
struct m_inode * new_inode(int dev)
{
	struct m_inode * inode;
	struct super_block * sb;
	struct buffer_head * bh;
	int i,j;

	// 从内存i节点列表（inode_table）中获取一个空闲i节点项（inode）。fs/inode.c中
	if (!(inode=get_empty_inode()))
		return NULL;

	// 获取超级块
	if (!(sb = get_super(dev)))
		panic("new_inode with unknown device");

	// 扫描i节点位图，寻找第一个0比特位，寻找空闲节点，获取放置该i节点的节点号
	j = 8192;
	for (i=0 ; i<8 ; i++)
		if (bh=sb->s_imap[i])
			if ((j=find_first_zero(bh->b_data))<8192)
				break;
	// 置位对应新i节点的i节点位图所在缓冲块无效（bh=NULL）， 则返回0. 退出（没有空闲节点）。
	if (!bh || j >= 8192 || j+i*8192 > sb->s_ninodes) {
		iput(inode);
		return NULL;
	}

	// 置位对应新i节点的i节点位图相应比特位，如果已经置位，则出错
	if (set_bit(j,bh->b_data))
		panic("new_inode: bit already set");
	bh->b_dirt = 1;			// 已修改标记
	inode->i_count=1;		// 引用计数
	inode->i_nlinks=1;		// 文件目录项链接数
	inode->i_dev=dev;		// i节点对应的设备号
	inode->i_uid=current->euid;	// 用户id
	inode->i_gid=current->egid;	// 组id
	inode->i_dirt=1;			// 已修改标记
	inode->i_num = j + i*8192;	// 对应设备中的i节点
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME; // 设置时间
	return inode;
}
