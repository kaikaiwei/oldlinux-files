/*
 *  linux/kernel/blk_dev/ll_rw.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This handles all read/write requests to block devices
 * 用于执行底层块设备读写操作。是所有块设备与系统其他部分的接口程序。
 * 其他程序通过调用ll_rw_block来读写块设备中的数据。
 */
#include <errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

#include "blk.h"

/*
 * The request-struct contains all necessary data
 * to load a nr of sectors into memory
 */
struct request request[NR_REQUEST]; // NR_REQUEST 32， 定义在blk.h中。请求队列

/*
 * used to wait on when there are no free requests  等待队列。
 * 请求数组中没有空闲项的临时等待队列
 */
struct task_struct * wait_for_request = NULL;

/* blk_dev_struct is:    磁盘设备类型
 *	do_request-address	 主设备的请求处理函数地址
 *	next-request		 主设备的请求队列
 */
struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */
	{ NULL, NULL },		/* dev mem 内存设备 */
	{ NULL, NULL },		/* dev fd  软盘设备*/
	{ NULL, NULL },		/* dev hd  硬盘设备*/
	{ NULL, NULL },		/* dev ttyx ttyx设备 */
	{ NULL, NULL },		/* dev tty  tty设备 */
	{ NULL, NULL }		/* dev lp   lp打印机设备*/
};

/**
 * 锁定buffer。
 * 就是在关闭中断的情况下。将buffer_head的b_lock置为1
 */
static inline void lock_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock)
		sleep_on(&bh->b_wait);
	bh->b_lock=1;
	sti();
}

/**
 * 释放buffer。
 * 就是在关闭中断的情况下。将buffer_head的b_lock置为0
 */
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk("ll_rw_block.c: buffer not locked\n\r");
	bh->b_lock = 0;
	wake_up(&bh->b_wait); // 唤醒等待的队列上的任务
}

/*
 * 添加一个请求
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 * @param dev 设备号，指定块设备
 * @param req 请求项
 * 向链表中加入一项请求。 他关闭中断，这样就能安全地处理请求链表了。
 */
static void add_request(struct blk_dev_struct * dev, struct request * req)
{
	struct request * tmp;

	req->next = NULL;	// 下一个等待项为null。防止队列无序扩张
	cli();	// 关闭中断
	if (req->bh)			// 有缓冲区头，那么清缓冲区脏标记
		req->bh->b_dirt = 0;
	if (!(tmp = dev->current_request)) {	// 第一项请求。 
		dev->current_request = req;			// 将设备块当前请求指针指向req。
		sti();								// 开中断
		(dev->request_fn)();				// 执行设备请求函数
		return;
	}

	// tmp这里是当前请求。通过电梯算法找到合适的地方。
	for ( ; tmp->next ; tmp=tmp->next)
		if ((IN_ORDER(tmp,req) ||
		    !IN_ORDER(tmp,tmp->next)) &&
		    IN_ORDER(req,tmp->next))
			break;
	// 链表插入操作
	req->next=tmp->next;
	tmp->next=req;
	sti();			// 开中断
}

/**
 * 生成请求
 * @param major 主设备号
 * @param rw  读写。
 * @param bh， buffer_head， 缓冲区头
 */
static void make_request(int major,int rw, struct buffer_head * bh)
{
	struct request * req;
	int rw_ahead;		// 预读标记

/* WRITEA/READA is special case - it is not really needed, so if the */
/* buffer is locked, we just forget about it, else it's a normal read */
	// 预读的情况
	if (rw_ahead = (rw == READA || rw == WRITEA)) {
		// 缓冲区头已经锁定
		if (bh->b_lock)
			return;
		// 更新读写标记
		if (rw == READA)
			rw = READ;
		else
			rw = WRITE;
	}
	// 如果读写标记不合法
	if (rw!=READ && rw!=WRITE)
		panic("Bad block dev command, must be R/W/RA/WA");

	// 锁定缓冲区头
	lock_buffer(bh);
	// 写请求，数据不脏
	// 读请求，数据已经是最新的
	if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) {
		unlock_buffer(bh);
		return;
	}
repeat:
/* we don't allow the write-requests to fill up the queue completely:
 * we want some room for reads: they take precedence. The last third
 * of the requests are only for reads.
 */
	// 读请求可以拥有整个请求数组，但是写请求只能使用数据的2/3.
	if (rw == READ)
		req = request+NR_REQUEST;
	else
		req = request+((NR_REQUEST*2)/3);
	/* find an empty request 查找空请求。 */
	while (--req >= request)
		if (req->dev<0)
			break;
	/* if none found, sleep on new requests: check for rw_ahead */
	// 如果没有找到。 在wait_for_request上睡眠
	if (req < request) {
		if (rw_ahead) {
			unlock_buffer(bh);
			return;
		}
		sleep_on(&wait_for_request);
		goto repeat;
	}
	
	// 找到了，就填充数据。
/* fill up the request-info, and add it to the queue */
	req->dev = bh->b_dev;			// 设备号
	req->cmd = rw;					// 读写命令
	req->errors=0;					// 出错次数标记
	req->sector = bh->b_blocknr<<1;	// 起始扇区（1块=2扇区）
	req->nr_sectors = 2;			// 读写扇区数
	req->buffer = bh->b_data;		// 数据缓冲区
	req->waiting = NULL;			// 任务等待操作执行完成的地方
	req->bh = bh;					// 缓冲区头指针
	req->next = NULL;				// 下一个请求项
	// 添加请求到对应设备的请求队列上
	add_request(major+blk_dev,req);
}

/**
 * 为块设备创建块设备请求项。并插入到指定块设备请求队列中。
 * 实际读写与欧设备的请求项处理函数request_fn完成。对于硬盘操作，是do_hd_request。do_fd_request, do_rd_request.
 * 
 */
void ll_rw_block(int rw, struct buffer_head * bh)
{
	unsigned int major;

	// 校验主设备号和设备的请求函数是否存在。
	if ((major=MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
	!(blk_dev[major].request_fn)) {
		printk("Trying to read nonexistent block-device\n\r");
		return;
	}
	// 生成请求。
	make_request(major,rw,bh);
}

/**
 * 块设备初始化函数
 * 就是将request请求队列都给置空
 */
void blk_dev_init(void)
{
	int i;

	for (i=0 ; i<NR_REQUEST ; i++) {
		request[i].dev = -1;	// 每一个请求的初始化值为-1，表示空闲
		request[i].next = NULL;	// 下一个指针为空
	}
}
