/*
 *  linux/fs/buffer.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting a interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it. NOTE! As interrupts
 * can wake up a caller, some cli-sti sequences are needed to check for
 * sleep-on-calls. These should be extremely quick, though (I hope).
 * 用来实现高速缓存功能。 不让中断过程改变缓冲区，而让调用者执行，从而避免竞争条件。
 * 注意，由于中断可以唤醒调用者，所以需要关中断和开中断序列来等待调用返回。
 * 这应该非常快，希望如此
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 * 检查软盘的程序不应该在这里。但是没有比这里更好的地方了。
 */

#include <stdarg.h>
 
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/io.h>

extern int end;				// 由连接程序ld生成的表明程序（内核程序）末端的变量
struct buffer_head * start_buffer = (struct buffer_head *) &end;
struct buffer_head * hash_table[NR_HASH];			// NR_HASH=107
static struct buffer_head * free_list;				// 空闲列表
static struct task_struct * buffer_wait = NULL;		// 等待队列
int NR_BUFFERS = 0;									// 缓冲区数量

/**
 * 等待指定缓冲区解锁
 * @param bh 缓冲区头
 */
static inline void wait_on_buffer(struct buffer_head * bh)
{
	cli();			// 关中断
	while (bh->b_lock)		// 如果在上锁
		sleep_on(&bh->b_wait);	// 就等待在缓冲区头的wait上。
	sti();			// 开中断
}

/**
 * sync 系统调用，同步设备和内存高速缓冲中数据
 */
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;

	sync_inodes();		/* write out inodes into buffers */ // 将i节点写入高度缓冲中

	// 遍历整个高速缓冲区，对于已经被修改的缓冲块，产生写盘请求，将缓冲区中数据与设备中同步
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);	// 等待解锁
		if (bh->b_dirt)	// 如果高速缓冲中数据比设备中新
			ll_rw_block(WRITE,bh);	// 写入设备
	}
	return 0;
}

/**
 * 对指定设备惊醒高速缓冲数据与设备上数据的同步操作
 */
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	// 遍历整个高速缓冲区
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)	// 跳过非指定设备的缓冲区块
			continue;
		wait_on_buffer(bh); // 获取缓冲区锁
		// 由于有唤醒，所以要重新检查设备号和脏标记
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh); // 写盘
	}
	sync_inodes();  // 同步inode节点信息

	// 再次同步设备信息
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

/**
 * 对指定设备的所有缓冲区无效
 */
void inline invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;

	// 遍历整个高速缓冲区
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)	// 跳过非指定设备
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev)	// 再次检查设备号
			bh->b_uptodate = bh->b_dirt = 0;	// 更新标志和脏标志置0，表示没有更新，也没有从设备同步到新的
	}
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that floppies/whatever are
 * special.
 * 键盘软盘修改
 * @param dev 更换前的设备号
 */
void check_disk_change(int dev)
{
	int i;

	// 不是软盘设备，跳过
	if (MAJOR(dev) != 2)
		return;
	// 如果软盘没有更换，那么就返回。
	if (!floppy_change(dev & 0x03))
		return;
	// 软盘更换了，
	for (i=0 ; i<NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);	// dev的所有inode缓存无效
	invalidate_buffers(dev); // dev的所有高速缓存无效
}

// 散列表项计算，计算index，NR_HASH=307
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
// 散列函数
#define hash(dev,block) hash_table[_hashfn(dev,block)]

/**
 * 从hash队列和空闲缓冲队列中移除
 * @param bh 缓冲区头
 */
static inline void remove_from_queues(struct buffer_head * bh)
{
	/* remove from hash-queue */
	// 从Hash队列中移除
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;	

	/* remove from free list */
	// 从空闲列表中移除
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	// 空闲列表头的处理
	if (free_list == bh)
		free_list = bh->b_next_free;
}

/**
 * 插入到空闲链表尾部并放入散列队列中
 * @param bh 缓冲区链表头
 */
static inline void insert_into_queues(struct buffer_head * bh)
{
	/* put at end of free list */
	// 插入到空闲列表中
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;

	/* put the buffer in new hash-queue if it has a device */
	// 如果该缓冲区对应一个设备，加入到新的散列队列中
	bh->b_prev = NULL;
	bh->b_next = NULL;
	// 如果不是对应一个设备，则退出
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;
}

/**
 * 查到指定设备和块号的缓冲区头
 * @param dev 	设备号
 * @param block 块号
 */
static struct buffer_head * find_buffer(int dev, int block)
{		
	struct buffer_head * tmp;

	// 通过hash链表查找
	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
			return tmp;
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 * 由于竞争条件。没有对缓冲区上锁（除非我们正在读他们中的数据），那么当进程睡眠时，
 * 缓冲区可能会发生一些问题（例如：读错误导致该缓冲区出错）。
 */
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;

	// 死循环
	for (;;) {
		// 没有找到缓冲区头
		if (!(bh=find_buffer(dev,block)))
			return NULL;
		bh->b_count++;	// 增加引用计数
		wait_on_buffer(bh);	// 等待缓冲区解锁
		// 睡眠等待，再次检查设备号和块号
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;
		// 该缓冲区所属的设备号或块号在睡眠时发生了改变，减少它的引用计数
		bh->b_count--;
	}
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 * 下面是getblk函数，由于要处理竞争条件，所以理解起来有点困难。
 */
// 判断缓冲区的修改标志和锁定标志，并且定义修改标志的权重比锁定标志大
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
/**
 * 获取高速缓冲中指定的缓冲区
 * @param dev 设备号
 * @param block 块号
 */
struct buffer_head * getblk(int dev,int block)
{
	struct buffer_head * tmp, * bh;

repeat:
	// 搜索散列表，如果指定块已经在高速缓冲中，则返回对应缓冲区头指针，退出
	if (bh = get_hash_table(dev,block))
		return bh;
	// 查找空闲缓冲区
	tmp = free_list;
	do {
		// 引用计数不为0， 表示正在使用中
		if (tmp->b_count)
			continue;
		// 缓冲区头指针bh为空，tmp的修改标志锁标志比bh的大，也就意味着权重比较大
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {
			bh = tmp;
			if (!BADNESS(tmp)) // 没有修改标志和锁标志，意味着已经为设备上的块取得了高速缓冲。
				break;
		}
/* and repeat until we find something good */
	} while ((tmp = tmp->b_next_free) != free_list); // 遍历空闲链表

	// 如果bh为空
	if (!bh) {
		sleep_on(&buffer_wait); // 在高速缓冲区上等待，然后重新执行
		goto repeat;
	}
	// bh不为空，等待解锁
	wait_on_buffer(bh); 
	// 解锁后，依旧被使用，重新查找
	if (bh->b_count)
		goto repeat;

	// 如果缓冲区头有更新标志
	while (bh->b_dirt) {
		// 发出sync指令，进行写盘操作
		sync_dev(bh->b_dev);
		// 等待解锁
		wait_on_buffer(bh);
		// 解锁后，依旧在使用中，重新查找
		if (bh->b_count)
			goto repeat;
	}

	// 在高速缓冲区中查找该块号是否已经加入。如果是，那么就重新查找
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
	if (find_buffer(dev,block))
		goto repeat;
	// 初始化缓冲区头信息
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	bh->b_count=1;
	bh->b_dirt=0;
	bh->b_uptodate=0;
	remove_from_queues(bh);	// 从hash队列和free列表中移除
	bh->b_dev=dev;			// 设备号
	bh->b_blocknr=block;	// 块号
	insert_into_queues(bh);	// 插入到空闲列表尾部并放入到hash队列中。 根据新的设备号和块号重新插入
	return bh;
}

/**
 * 释放缓冲区
 * @param buf 缓冲区头
 */
void brelse(struct buffer_head * buf)
{
	// 缓冲区头不合法
	if (!buf)
		return;
	// 等待解锁
	wait_on_buffer(buf);
	// 如果缓冲区还在使用中，死机
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	// 唤醒等待的进程，task_struct
	wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 * 读取指定设备号和块号的内容
 * @param dev 设备号
 * @param block 块号
 * @return buffer_head 缓冲区头或NULL
 */
struct buffer_head * bread(int dev,int block)
{
	struct buffer_head * bh;

	// 获取高速缓冲区
	if (!(bh=getblk(dev,block)))
		panic("bread: getblk returned NULL\n");
	// 如果是刚从磁盘读取的
	if (bh->b_uptodate)
		return bh;
	// 进行读操作，将磁盘数据读取到bh中。 产生读设备块请求
	ll_rw_block(READ,bh);
	// 等待bh解锁
	wait_on_buffer(bh);
	// 如果已经读取到数据，那么就返回bh
	if (bh->b_uptodate)
		return bh;
	// 没有读取到，释放bh
	brelse(bh);
	// 返回空
	return NULL;
}

// 复制内存块，从from地址到to地址。 ams汇编
#define COPYBLK(from,to) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"movsl\n\t" \
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
	:"cx","di","si")

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 * 一次从设备上读取指定4个缓冲块到指定的地址。
 * 同时读取4块可以获取速度上的好处。
 * @param address 读取到的地址
 * @param dev 设备号
 * @param b 4个块号
 */
void bread_page(unsigned long address,int dev,int b[4])
{
	struct buffer_head * bh[4];
	int i;

	// 一次读取4个缓存区块
	for (i=0 ; i<4 ; i++)
		if (b[i]) { // 块号不为0
			// 读取缓冲区块
			if (bh[i] = getblk(dev,b[i]))
				if (!bh[i]->b_uptodate) // 不是最新的
					ll_rw_block(READ,bh[i]); // 发起读请求进行读取
		} else
			bh[i] = NULL;
	
	// 对于读取到的4个缓冲区块。
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
		if (bh[i]) {
			// 等待释放
			wait_on_buffer(bh[i]);
			// 如果是最新的，那么就拷贝到address+offset的地址处
			if (bh[i]->b_uptodate)
				COPYBLK((unsigned long) bh[i]->b_data,address);
			// 释放缓冲区头
			brelse(bh[i]);
		}
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 * breada可以像bread一样使用，但是会另外欲读一些块。该函数参数列表需要使用一个负数来表明参数列表的结束
 * 从指定设备读取指定的一些块。成功返回第1块的缓冲区头指针。否则返回NULL
 * @param dev 设备号
 * @param first 第一个块号
 */
struct buffer_head * breada(int dev,int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;

	va_start(args,first);
	// 获取缓冲区块
	if (!(bh=getblk(dev,first)))
		panic("bread: getblk returned NULL\n");
	// 如果不是最新的，那么就发起读取命令
	if (!bh->b_uptodate)
		ll_rw_block(READ,bh);
	// 获取剩余参数
	while ((first=va_arg(args,int))>=0) {
		// 获取缓冲区块
		tmp=getblk(dev,first);
		if (tmp) {
			// 如果不是最新的，就发起读取命令
			if (!tmp->b_uptodate)
				ll_rw_block(READA,bh); // 发起读取命令。 这里的bh应该是tmp，在0.96版本被纠正
			tmp->b_count--; // 不进行引用
		}
	}
	va_end(args);
	// 如果bh加锁，那么就解锁
	wait_on_buffer(bh); 
	// 如果数据是最新的，那么就返回
	if (bh->b_uptodate)
		return bh;
	// 否则，释放bh，并返回NULL
	brelse(bh);
	return (NULL);
}

/**
 * 高速缓冲区初始化
 * @param buffer_end 时指定缓冲区内存的末端。 如果系统有16MB内存，则缓冲区末端设置为4MB。
 * 		如果系统有8MB内存，则缓冲区末端设置为2MB。
 */
void buffer_init(long buffer_end)
{
	struct buffer_head * h = start_buffer; // 缓冲开始地址。 内核程序末端
	void * b;
	int i;

	// 如果缓存区高端地址等于1MB， 由于640KB-1MB被显示内存和BIOS暂用，因此实际可使用缓冲区内存高端应该是640KB
	// 否则内存高端地址一定大于1MB。
	if (buffer_end == 1<<20)
		b = (void *) (640*1024);
	else
		b = (void *) buffer_end;
	
	// 初始化缓冲区，建立空闲缓冲区循环链表，并获得系统中缓冲块的数据。
	// 从缓冲区高端地址开始，划分1KB大小的缓冲块。 同时在缓冲区低端建立描述该缓冲块的结构，
	// 并将这些buffer_head组成双向链表。h指向缓冲区头结构的指针
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) { // fs.h中定义BLOCK_SIZE为1024
		h->b_dev = 0;
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;	// 指向链表的缓冲区块
		h->b_prev_free = h-1;
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;	// 缓冲区计数
		// 如果地址b递减到等于1MB，则跳过384KB
		if (b == (void *) 0x100000)
			b = (void *) 0xA0000; // 让B指向0xA0000，640KB处
	}
	h--; // 让h执行最后一个有效缓冲区头
	free_list = start_buffer;	// 空闲列表指向第一个有效缓冲区头
	free_list->b_prev_free = h;	// 空闲列表的前一个指向最后一个
	h->b_next_free = free_list;	// 最后一个有效缓冲区头的
	// hash table初始化为NULL。 
	for (i=0;i<NR_HASH;i++)
		hash_table[i]=NULL;
}	
