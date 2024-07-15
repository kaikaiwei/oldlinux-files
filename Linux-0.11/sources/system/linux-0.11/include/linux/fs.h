/*
 * This file has definitions for some important file table
 * structures etc.
 */

#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

/* devices are as follows: (same as minix, so we can use the minix
 * file system. These are major numbers.)
 *
 * 0 - unused (nodev)  未使用
 * 1 - /dev/mem			内存设备
 * 2 - /dev/fd			软盘设备
 * 3 - /dev/hd			硬盘设备
 * 4 - /dev/ttyx		tty串行终端设备
 * 5 - /dev/tty			tty终端设备
 * 6 - /dev/lp			打印设备
 * 7 - unnamed pipes	未命名管道
 */

// 是不是可以寻找定位的设备
#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)

// 读写定义。
#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

// 高速缓冲区初始化
void buffer_init(long buffer_end);

// 获得设备的主设备号
#define MAJOR(a) (((unsigned)(a))>>8)
// 获得设备的次设备号
#define MINOR(a) ((a)&0xff)

// 名字长度
#define NAME_LEN 14
// 根i节点
#define ROOT_INO 1

// i节点位图槽数
#define I_MAP_SLOTS 8
// 逻辑块（区段块）位图槽数
#define Z_MAP_SLOTS 8
// 文件系统魔数
#define SUPER_MAGIC 0x137F

// 打开文件数
#define NR_OPEN 20
#define NR_INODE 32
#define NR_FILE 64
#define NR_SUPER 8
#define NR_HASH 307
#define NR_BUFFERS nr_buffers
// 数据块长度
#define BLOCK_SIZE 1024
// 数据款长度所占比特位数
#define BLOCK_SIZE_BITS 10
// NULL的定义
#ifndef NULL
#define NULL ((void *) 0)
#endif

// 每个逻辑块可存放的i节点数
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))
// 每个逻辑块可存放的目录项数
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

// 获取管道头指针
#define PIPE_HEAD(inode) ((inode).i_zone[0])
// 获取管道尾指针
#define PIPE_TAIL(inode) ((inode).i_zone[1])
// 管道大小
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
// 管道空
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
// 管道满
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))
// 管道头指针递增
#define INC_PIPE(head) \
__asm__("incl %0\n\tandl $4095,%0"::"m" (head))

// 块缓冲区
typedef char buffer_block[BLOCK_SIZE];

/**
 * 缓冲区头指针
 */
struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */ // 指向数据区
	unsigned long b_blocknr;	/* block number */
	unsigned short b_dev;		/* device (0 = free)  设备号 */
	unsigned char b_uptodate;
	unsigned char b_dirt;		/* 0-clean,1-dirty  脏标记*/
	unsigned char b_count;		/* users using this block 使用计数*/
	unsigned char b_lock;		/* 0 - ok, 1 -locked  锁标记 */
	struct task_struct * b_wait;		// 等待队列
	struct buffer_head * b_prev;		// 前一个
	struct buffer_head * b_next;		// 后一个
	struct buffer_head * b_prev_free;	// 前一个空闲
	struct buffer_head * b_next_free;	// 后一个空闲
};

// i节点， 磁盘上的索引节点数据结构
struct d_inode {
	unsigned short i_mode;	// 文件类型和属性，rwx位
	unsigned short i_uid;	// 用户id，文件拥有者表示符
	unsigned long i_size;	// 文件大小，字节数
	unsigned long i_time;	// 修改时间
	unsigned char i_gid;	// 文件拥有者所在的组
	unsigned char i_nlinks;	// 链接数，有多少文件目录项指向该i节点
	unsigned short i_zone[9];	// 区段，逻辑块
};

// 内存中的i节点结构，前7项与d_inode完全一样。
struct m_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_mtime;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];	// 0-6是直接索引，7是间接索引，8是双间接索引
/* these are in memory also */
	struct task_struct * i_wait;	// 等待该i节点的进程
	unsigned long i_atime;			// 最后访问时间
	unsigned long i_ctime;			// i节点所在的设备号
	unsigned short i_dev;			// i节点所在的设备号
	unsigned short i_num;			// i节点号
	unsigned short i_count;			// i节点被使用的次数，0表示该i节点空闲
	unsigned char i_lock;			// 锁定标记
	unsigned char i_dirt;			// 脏标记，已修改标记
	unsigned char i_pipe;			// 管道标志
	unsigned char i_mount;			// 安装标志
	unsigned char i_seek;			// 搜寻标志 lseek的时候
	unsigned char i_update;			// 更新标志
};

/**
 * 文件数据结构
 */
struct file {
	unsigned short f_mode;		// 文件操作模式，rw位
	unsigned short f_flags;		// 文件打开和控制标记
	unsigned short f_count;		// 对应文件句柄数
	struct m_inode * f_inode;	// 指向对应的i节点
	off_t f_pos;				// 文件偏移量
};

/**
 * 
 */
struct super_block {
	unsigned short s_ninodes;		// 节点数
	unsigned short s_nzones;		// 逻辑块数
	unsigned short s_imap_blocks;	// i节点位图所占用的数据块数
	unsigned short s_zmap_blocks;	// 逻辑块位图所占用的数据块数
	unsigned short s_firstdatazone;	// 第一个数据逻辑块号
	unsigned short s_log_zone_size;	// log（数据块数/逻辑块数），以2为底
	unsigned long s_max_size;		// 文件最大长度
	unsigned short s_magic;			// 文件系统魔数
/* These are only in memory */
	struct buffer_head * s_imap[8];	// i节点位图缓冲块指针数组（占用8块，可表示64M）
	struct buffer_head * s_zmap[8];	// 逻辑块位图缓冲块指针数组（占用8块）
	unsigned short s_dev;			// 超级块所在的设备号
	struct m_inode * s_isup;		// 被安装的文件系统根目录的i节点
	struct m_inode * s_imount;		// 被安装到的i节点
	unsigned long s_time;			// 修改时间
	struct task_struct * s_wait;	// 等待该超级块的进程
	unsigned char s_lock;			// 被锁定标志
	unsigned char s_rd_only;		// 只读标志
	unsigned char s_dirt;			// 已修改标志
};

// 磁盘上的超级块结构
struct d_super_block {
	unsigned short s_ninodes;		// 节点数
	unsigned short s_nzones;		// 逻辑块数
	unsigned short s_imap_blocks;	// i节点位图所占用的数据块数
	unsigned short s_zmap_blocks;	// 逻辑块位图所占用的数据块数
	unsigned short s_firstdatazone;	// 第一个数据逻辑块
	unsigned short s_log_zone_size;	// log（数据块数/逻辑块数），以2为底
	unsigned long s_max_size;		// 文件最大长度
	unsigned short s_magic;			// 文件系统魔数
};

// 文件目录项结构
struct dir_entry {
	unsigned short inode;	// i节点
	char name[NAME_LEN];	// 文件名
};

// 定义i节点表数组（32项）
extern struct m_inode inode_table[NR_INODE];
// 定义文件表数组，64项
extern struct file file_table[NR_FILE];
// 超级块数据，8项
extern struct super_block super_block[NR_SUPER];
// 缓冲区起始内存位置
extern struct buffer_head * start_buffer;
// 缓冲块数
extern int nr_buffers;

// 检测驱动器中软盘是否改变
extern void check_disk_change(int dev);
// 检测指定软驱中软盘更换情况。如果软盘更换了则返回1，否则返回0
extern int floppy_change(unsigned int nr);
// 设置启动指定驱动器所需等待的额时间（设置等待定时器）
extern int ticks_to_floppy_on(unsigned int dev);
// 启动指定软盘驱动器
extern void floppy_on(unsigned int dev);
// 关闭指定软盘驱动器
extern void floppy_off(unsigned int dev);
// 将i节点指定的文件截为0
extern void truncate(struct m_inode * inode);
// 刷新i节点信息
extern void sync_inodes(void);
// 等待指定的i节点
extern void wait_on(struct m_inode * inode);
// 逻辑块（区段，磁盘块）位图操作，取数据块block在设备上对应的逻辑块号
extern int bmap(struct m_inode * inode,int block);
// 创建数据块clock在设备上对应的逻辑块，并返回在设备上的逻辑块号
extern int create_block(struct m_inode * inode,int block);
// 获取指定路径名的i节点号
extern struct m_inode * namei(const char * pathname);
// 根据路径名为文件打开操作做准备
extern int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);
// 释放一个i节点，回写入设备
extern void iput(struct m_inode * inode);
// 从设备中读取指定节点号的一个i节点
extern struct m_inode * iget(int dev,int nr);
// 从i节点表中获取一个空闲i节点项
extern struct m_inode * get_empty_inode(void);
// 获取管道节点，返回i节点指针
extern struct m_inode * get_pipe_inode(void);
// 在哈希表中查找指定的数据块，返回找到块的缓冲头指针
extern struct buffer_head * get_hash_table(int dev, int block);
// 从设备读块（现在has表中查找）
extern struct buffer_head * getblk(int dev, int block);
// 读写数据块
extern void ll_rw_block(int rw, struct buffer_head * bh);
// 释放指定数据块
extern void brelse(struct buffer_head * buf);
// 读取指定的数据块
extern struct buffer_head * bread(int dev,int block);
// 读4块缓冲区到内存中
extern void bread_page(unsigned long addr,int dev,int b[4]);
// 读取头一个指定的数据块，并标记后续将要读的块（预读）
extern struct buffer_head * breada(int dev,int block,...);
// 向设备dev申请一磁盘块，并返回逻辑块号
extern int new_block(int dev);
// 释放设备数据区中的逻辑块block。 复位指定逻辑块block的逻辑块位图比特位
extern void free_block(int dev, int block);
// 为设备dev建议一i节点，返回i节点号
extern struct m_inode * new_inode(int dev);
// 释放一个i节点（删除文件时）
extern void free_inode(struct m_inode * inode);
// 刷新指定设备缓冲区
extern int sync_dev(int dev);
// 读取指定设备的超级块
extern struct super_block * get_super(int dev);
// 启动引导时的根文件系统设备号
extern int ROOT_DEV;

// 安装根文件系统
extern void mount_root(void);

#endif
