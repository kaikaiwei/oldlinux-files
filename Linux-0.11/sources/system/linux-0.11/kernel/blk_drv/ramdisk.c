/*
 *  linux/kernel/blk_drv/ramdisk.c
 *
 *  Written by Theodore Ts'o, 12/2/91
 */

#include <string.h>

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/memory.h>

#define MAJOR_NR 1	// 内存的主设备号
#include "blk.h"

char	*rd_start;	// 虚拟盘初始化位置
int	rd_length = 0;	// 虚拟盘长度，占用内存的大小

/**
 * 虚拟盘请求处理函数，执行虚拟盘读写操作
 * 
 */
void do_rd_request(void)
{
	int	len;			// 读写字节数
	char	*addr;		// 内存中的起始地址

	INIT_REQUEST;	// 请求合法行校验

	// 起始地址= 虚拟盘初始化位置+ 起始扇区*512
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->nr_sectors << 9; // 通过读写扇区数*512，得到读写字节长度

	// 校验主设备号和读写范围是否在虚拟盘中。
	if ((MINOR(CURRENT->dev) != 1) || (addr+len > rd_start+rd_length)) {
		end_request(0);
		goto repeat;
	}

	// 写请求，进行内存拷贝操作
	if (CURRENT-> cmd == WRITE) {
		// memcpy（写入地址，来源地址，写入长度）
		(void ) memcpy(addr,
			      CURRENT->buffer,
			      len);
	} else if (CURRENT->cmd == READ) {
		(void) memcpy(CURRENT->buffer, 
			      addr,
			      len);
	} else
		panic("unknown ramdisk-command");
	end_request(1);
	goto repeat;
}

/*
 * Returns amount of memory which needs to be reserved.
 * 虚拟盘初始化。
 * @param mem_start 虚拟盘在内存中的起始地址
 * @param length   虚拟盘长度。字节数
 */
long rd_init(long mem_start, int length)
{
	int	i;
	char	*cp;	// 临时存放，用于虚拟盘占据字节的清0

	// 设备列表中，初始化信息
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

	rd_start = (char *) mem_start;
	rd_length = length;
	cp = rd_start;

	// 内存信息清0.
	for (i=0; i < length; i++)
		*cp++ = '\0';
	return(length);
}

/*
 * If the root device is the ram disk, try to load it.
 * In order to do this, the root device is originally set to the
 * floppy, and we later change it to be ram disk.
 * 如果根文件系统设备是ram disk的 化，尝试加载它。
 * 加载根文件系统到ram disk
 */
void rd_load(void)
{
	struct buffer_head *bh;		// 高速缓存块头指针
	struct super_block	s;		// 超级快头指针
	int		block = 256;	/* Start at block 256 */
	int		i = 1;				// 表示根文件系统映像文件在boot盘第256磁盘块开始处
	int		nblocks;			// 
	char		*cp;		/* Move pointer */
	
	// 如果虚拟盘长度为0， 则退出
	if (!rd_length)
		return;
	// 打印虚拟盘起始地址和长度
	printk("Ram disk: %d bytes, starting at 0x%x\n", rd_length,
		(int) rd_start);

	// 如果此时根文件系统不是软盘，则退出
	if (MAJOR(ROOT_DEV) != 2)
		return;

	/*
	 * 读软盘256+1， 256， 256+2 的磁盘块
	 * breada 用于读取指定的磁盘块。 fs/buffer.c中。 
	 * 这里block+1是指超级块。
	 */
	bh = breada(ROOT_DEV,block+1,block,block+2,-1);
	if (!bh) {
		printk("Disk error while looking for ramdisk!\n");
		return;
	}

	// 转换为超级块的指针。 s复制磁盘块中的超级块
	*((struct d_super_block *) &s) = *((struct d_super_block *) bh->b_data);
	brelse(bh); // 释放缓冲区头

	// 超级快的魔数不对
	if (s.s_magic != SUPER_MAGIC) 
		/* No ram disk image present, assume normal floppy boot */
		return;
	
	// 逻辑块数 =  区段数 * 2 ^(每区段块数的幂)
	nblocks = s.s_nzones << s.s_log_zone_size;
	// 超出虚拟盘的容量
	if (nblocks > (rd_length >> BLOCK_SIZE_BITS)) {
		printk("Ram disk image too big!  (%d blocks, %d avail)\n", 
			nblocks, rd_length >> BLOCK_SIZE_BITS);
		return;
	}
	printk("Loading %d bytes into ram disk... 0000k", 
		nblocks << BLOCK_SIZE_BITS);
	
	// 开始内存拷贝
	cp = rd_start;
	while (nblocks) {
		// 预读
		if (nblocks > 2) 
			bh = breada(ROOT_DEV, block, block+1, block+2, -1);
		else
			bh = bread(ROOT_DEV, block);
		// 读取错误
		if (!bh) {
			printk("I/O error on block %d, aborting load\n", 
				block);
			return;
		}
		// 拷贝到cp指向的内存地址处
		(void) memcpy(cp, bh->b_data, BLOCK_SIZE);
		brelse(bh);
		printk("\010\010\010\010\010%4dk",i);
		
		// 修改临时变量
		cp += BLOCK_SIZE;
		block++;
		nblocks--;
		i++;
	}
	printk("\010\010\010\010\010done \n");
	ROOT_DEV=0x0101;	// 修改root_dev为虚拟盘
}
