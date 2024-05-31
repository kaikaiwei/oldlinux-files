#ifndef _BLK_H
#define _BLK_H

// 磁盘设备类型
#define NR_BLK_DEV	7
/*
 * 请求队列长度
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-mechanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too many (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
#define NR_REQUEST	32

/*
 * 磁盘请求
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
struct request {
	int dev;		/* -1 if no request */ 	// 请求的磁盘. 块设备号，-1表示这项空闲
	int cmd;		/* READ or WRITE */		// 请求的命令，读或者写
	int errors;								// 错误标志。操作时产生错误的次数
	unsigned long sector;					// 起始扇区（1块=2扇区）
	unsigned long nr_sectors;				// 读/写扇区数
	char * buffer;							// 数据缓冲区
	struct task_struct * waiting;			// 等待队列。 任务等待操作执行完成的地方
	struct buffer_head * bh;				// 缓存区头指针。include/linux/fs.h
	struct request * next;					// 下一个请求项
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 * 用于电梯算法。先比较cmd，在比较dev，在比较sector。
 */
#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector)))

/**
 * 块设备数据结构。
 */
struct blk_dev_struct {
	void (*request_fn)(void);			// 请求项操作的函数指针
	struct request * current_request;	// 当前请求项
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];	// 块设备表 NR_BLK_DEV=7
extern struct request request[NR_REQUEST];			// 请求队列数组，NR_REQUEST=32个
extern struct task_struct * wait_for_request;		// 等待队列

// 如果定义了MAJOR_NR， 主设备号。来自setup.s还是head.s
#ifdef MAJOR_NR

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 * 目前仅支持硬盘和软盘（还有虚拟盘）。 
 */
#if (MAJOR_NR == 1)		// RAM盘的主设备号为1. 内存块主设备号也是1
/* ram disk */
#define DEVICE_NAME "ramdisk"				// 名称
#define DEVICE_REQUEST do_rd_request		// 设备请求函数
#define DEVICE_NR(device) ((device) & 7)	// 设备号
#define DEVICE_ON(device) 					// 开启设备，虚拟盘无需开启和关闭
#define DEVICE_OFF(device)					// 关闭设备

#elif (MAJOR_NR == 2)						// 软盘，主设备号为2
/* floppy */
#define DEVICE_NAME "floppy"				// 设备名称
#define DEVICE_INTR do_floppy				// 设备中断处理函数
#define DEVICE_REQUEST do_fd_request		// 设备请求函数
#define DEVICE_NR(device) ((device) & 3)	// 设备号
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))	// 开启设备
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))	// 关闭设备

#elif (MAJOR_NR == 3)						// 硬盘，主设备号为3
/* harddisk */
#define DEVICE_NAME "harddisk"				// 设备名称
#define DEVICE_INTR do_hd					// 设备中断处理函数
#define DEVICE_REQUEST do_hd_request		// 设备请求函数
#define DEVICE_NR(device) (MINOR(device)/5)	// 设备号 0-1， 每个硬盘可以有4个分区
#define DEVICE_ON(device)					// 硬盘一直在工作，无需开启和关闭
#define DEVICE_OFF(device)

#elif
/* unknown blk device */
#error "unknown blk device"

#endif

// CURRENT为指定主设备号的当前请求结构
#define CURRENT (blk_dev[MAJOR_NR].current_request)
// CURRENT_DEV为CURRENT的设备号
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

// 如果定义了中断请求函数。声明函数
#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif

// 声明设备请求函数
static void (DEVICE_REQUEST)(void);

// 释放指定的缓冲区头
extern inline void unlock_buffer(struct buffer_head * bh)
{
	// 如果没有上锁
	if (!bh->b_lock)
		printk(DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock=0; // 表示没有上锁
	wake_up(&bh->b_wait); // 唤醒等待队列
}

/**
 * 结束请求
 * @param uptodate 更新
 */
extern inline void end_request(int uptodate)
{
	DEVICE_OFF(CURRENT->dev);	// 关闭设备
	
	// 如果有缓冲区头指针，更新缓冲区头指针的b_uptodate。 并释放该缓冲区头
	if (CURRENT->bh) {
		CURRENT->bh->b_uptodate = uptodate;
		unlock_buffer(CURRENT->bh);
	}

	// 如果更新标志为0，则显示设备错误信息
	if (!uptodate) {
		printk(DEVICE_NAME " I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}

	// 唤醒等待该请求项的进程
	wake_up(&CURRENT->waiting);
	// 唤醒等待请求的进程
	wake_up(&wait_for_request);

	CURRENT->dev = -1;	// 释放当前请求项
	CURRENT = CURRENT->next;	// 当前请求项为指向下一个
}

// 定义初始化请求宏
#define INIT_REQUEST \
repeat: \
	if (!CURRENT) \				// 如果当前请求结构指针为空，则返回
		return; \
	if (MAJOR(CURRENT->dev) != MAJOR_NR) \			// 如果当前设备的主设备号不对，则死机
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \				// 缓存区头指针存在
		if (!CURRENT->bh->b_lock) \		// 缓存区头指针没有锁定，就死机
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif

#endif
