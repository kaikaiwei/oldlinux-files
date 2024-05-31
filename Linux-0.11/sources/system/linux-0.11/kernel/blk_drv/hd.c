/*
 *  linux/kernel/hd.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This is the low-level hd interrupt support. It traverses the
 * request-list, using interrupts to jump between functions. As
 * all the functions are called within interrupts, we may not
 * sleep. Special care is recommended.
 * 
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.
 * 这里是磁盘控制器控制程序。 提供对硬盘控制器块设备的读写驱动和硬盘初始化处理。分为5类
 * 初始化硬盘和设备硬盘所用数据结构信息的函数，sys_setup和hd_init
 * 向硬盘控制器发行命令的函数hd_out
 * 处理硬盘当前请求项的函数do_hd_request
 * 硬盘中断处理过程中调用的c函数。read_intr，write_intr，bad_rw_intr，recal_intr.
 * 硬盘控制器辅助函数。 controller_ready，drive_busy，win_result，hd_out，reset_controller。
 */

#include <linux/config.h>	// 内核配置
#include <linux/sched.h>	// 内核调度
#include <linux/fs.h>		// 文件系统头文件
#include <linux/kernel.h>	// 内核头文件
#include <linux/hdreg.h>	// 硬盘参数头文件。定义访问硬盘寄存器端口，状态码，分区表等信息
#include <asm/system.h>		// 系统头文件。 
#include <asm/io.h>			// io头文件
#include <asm/segment.h>	// 段操作头文件

// 主设备好
#define MAJOR_NR 3
#include "blk.h"

// 读CMOS参数宏函数
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector 
 * MAX_ERRORS 读写一个扇区时允许的最多出错次数
 * MAX_HD 系统支持的最多硬盘数
 */
#define MAX_ERRORS	7
#define MAX_HD		2

// 硬盘中断程序在复位操作时会调用的重新校正函数
static void recal_intr(void);

static int recalibrate = 1;	// 重新矫正标志。将磁头移动到0柱面
static int reset = 1;		// 复位标志

/*
 *  This struct defines the HD's and their types.
 * 硬盘参数及类型
 * head， 磁头数
 * sect， 每磁道扇区数
 * cyl， 柱面数
 * wpcom， 写前预补偿柱面号
 * lzone， 磁头着陆柱面号
 * ctl， 控制字节
 */
struct hd_i_struct {
	int head,sect,cyl,wpcom,lzone,ctl;
	};

// 如果已经在include/linux/config.h中定义了hd_type
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };			// 取定义好的参数作为hd_info
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))	// 计算硬盘数
#else
// 否则，都预设为0
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;
#endif

/**
 * 硬盘分区结构
 * start_sect 物理起始扇区号
 * nr_sects 扇区总数
 * 其中5的倍数的向，代表整个硬盘中的参数，hd[0],hd[5]等
 */
static struct hd_struct {
	long start_sect;
	long nr_sects;
} hd[5*MAX_HD]={{0,0},};

// 读port端口，读nr个字节，放入到buf中
#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")

// 写port端口，写nr个字节，从buf中取数据
#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")

// 外部函数定义
extern void hd_interrupt(void);	// system_call.s中
extern void rd_load(void);		// ramdisk.c中

/* This may be used only once, enforced by 'static int callable' 
 * 该函数值调用一次。
 * @param BIOS 是setup.s的时候取得硬盘信息。
 */
int sys_setup(void * BIOS)
{
	static int callable = 1; // 1表示未调用，0表示已经调用过。
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;

	// 如果已经调用过了， 那么就返回。
	if (!callable)
		return -1;
	callable = 0;

	// 如果没有定义HD_TYPE， include/linux/config.h。 就从0x90080处读入
#ifndef HD_TYPE
	for (drive=0 ; drive<2 ; drive++) {
		hd_info[drive].cyl = *(unsigned short *) BIOS;
		hd_info[drive].head = *(unsigned char *) (2+BIOS);
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);
		BIOS += 16;
	}
	// 判断第二个柱面是否为0，就可以知道有没有第二块硬盘。setup.s在取BIOS硬件参数的时候，如果只有1个硬盘，就会将第二块硬盘的16B都置位0.
	if (hd_info[1].cyl)
		NR_HD=2;	// 有第二块硬盘
	else
		NR_HD=1;	// 只有一块硬盘
#endif
	// 从HD_TYPE初始化hd_info
	for (i=0 ; i<NR_HD ; i++) {
		hd[i*5].start_sect = 0;			// 硬盘起始扇区号
		hd[i*5].nr_sects = hd_info[i].head*
				hd_info[i].sect*hd_info[i].cyl;	// 硬盘总扇区号
	}

	/*
		We querry CMOS about hard disks : it could be that 
		we have a SCSI/ESDI/etc controller that is BIOS
		compatable with ST-506, and thus showing up in our
		BIOS table, but not register compatable, and therefore
		not present in CMOS.

		Furthurmore, we will assume that our ST-506 drives
		<if any> are the primary drives in the system, and 
		the ones reflected as drive 1 or 2.

		The first drive is stored in the high nibble of CMOS
		byte 0x12, the second in the low nibble.  This will be
		either a 4 bit drive type or 0xf indicating use byte 0x19 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.

		Needless to say, a non-zero value means we have 
		an AT controller hard disk for that drive.

		
	*/
	// 检测是否AT控制器兼容的
	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	else
		NR_HD = 0;

	// 如果NR_HD = 0， 表示两块硬盘都不是AT兼容的。 NR_HD=1， 只清空第二块硬盘的信息
	for (i = NR_HD ; i < 2 ; i++) {
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	// 加载分区表
	// 读取每一个硬盘上第1块数据（第一个扇区有用），获取其分区表信息
	// 首先利用bread读取第一块数据（fs/buffer.c）， 参数0x300上硬盘的主设备号。
	// 然后根据磁盘第一个扇区位置0x1fe处的两个字节是否为55AA来判断该扇区位于0x1BE的分区表是否有效。
	// 最后将分区表信息放入硬盘分区表数据结构hd中。
	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive*5,0))) {
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;
		for (i=1;i<5;i++,p++) {
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);	// 是否磁盘缓冲区头。
	}

	// 硬盘存在，且已经读入分区表
	if (NR_HD)
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");

	rd_load();	// 加载RAMDISK kernel/blk_drv/ramdisk.c
	mount_root();	// 安装根文件系统
	return (0);
}

/**
 * 判断并循环等待驱动器就绪
 * 读硬盘控制器端口HD_STATUS（0x1f7）， 并循环检测驱动器就绪标志为和控制器忙位
 */
static int controller_ready(void)
{
	int retries=10000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

/**
 * 检测硬盘执行命令后的状态（win_ 表示温切斯特硬盘的缩写）
 * 读取状态寄存器中的命令执行结果状态。
 * 返回0 表示正常。 1 表示出错。
 * 如果执行命令错， 那么会读取错误寄存器HD_ERROR(0x1f1)
 */
static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	// 如果出错，读取错误状态寄存器
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

/**
 * 向硬盘控制器发送命令块。
 * @param drive 硬盘号， 0-1
 * @param nsect 读写扇区数
 * @param sect  起始扇区号
 * @param head  磁头号
 * @param cyl   柱面号
 * @param cmd   命令码
 * @param intr_addr  应用中断处理程序中将调用的c处理函数
 */
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
	register int port asm("dx");	// port 变量对应寄存器dx

	// 驱动器号大于1，或者磁头号大于15. 系统不支持
	if (drive>1 || head>15)
		panic("Trying to write bad sector");

	// 如果等待一段时间后仍未就绪则死机
	if (!controller_ready())
		panic("HD controller not ready");
	// do_hd执行硬盘中断程序中被调用
	do_hd = intr_addr;
	outb_p(hd_info[drive].ctl,HD_CMD);	// 向控制寄存器（0x3f6）输出控制字节
	port=HD_DATA;						// 端口号，dx为数据寄存器端口 0x1f0
	outb_p(hd_info[drive].wpcom>>2,++port);	// 参数，写预补偿柱面号，需要除以4
	outb_p(nsect,++port);					// 参数，读写扇区总数
	outb_p(sect,++port);					// 参数，起始扇区
	outb_p(cyl,++port);						// 参数，柱面号低8位
	outb_p(cyl>>8,++port);					// 参数，柱面号高8位
	outb_p(0xA0|(drive<<4)|head,++port);	// 参数，驱动器号，+ 磁头号
	outb(cmd,++port);						// 命令，硬盘控制命令
}

/**
 * 等待磁盘就绪。
 * 也就是循环等待主状态控制器忙标志复位。 若仅有就绪或者寻到结束标志置位，则成功，返回0。 
 * 若经过一段时间等待仍为忙，则返回1.
 */
static int drive_busy(void)
{
	unsigned int i;

	// 循环执行一万次，等待就绪标志为复位
	for (i = 0; i < 10000; i++)
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
			break;
	// 再取主控制器状态字节
	i = inb(HD_STATUS);
	// 检测忙位，就绪位和寻道结束位
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;
	if (i == READY_STAT | SEEK_STAT)	// 如果仅有就绪或寻道结束标志，则返回0
		return(0);
	printk("HD controller times out\n\r");
	return(1);
}

/**
 * 复位硬盘控制器
 */
static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);						// 向控制寄存器端口发送控制字节（4表示复位）
	for(i = 0; i < 100; i++) nop();		// 循环等待一段时间
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);	// 发送正常的控制字节（不禁止重试，重读）。

	// 若等待硬盘就绪超时，则显示出错信息
	if (drive_busy())
		printk("HD-controller still busy\n\r");

	// 如果错误寄存器不为1， 则出错
	if ((i = inb(HD_ERROR)) != 1)
		printk("HD-controller reset failed: %02x\n\r",i);
}

/**
 * 复位硬盘nr。
 * 首先复位硬盘控制器。然后发送硬盘控制器命令。 建立驱动器参数。
 * 其中recal_intr是在硬盘中断处理程序中调用的重新校正处理函数。
 */
static void reset_hd(int nr)
{
	reset_controller();
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

/**
 * 意外硬盘中断调用函数
 * 发生意外硬盘中断时，硬盘中断处理程序中调用的默认c处理函数。
 * 在被调用函数指针位空时调用该函数。 参见kernel/system_call.s
 */
void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
}

/**
 * 读写硬盘失败处理调用函数
 */
static void bad_rw_intr(void)
{
	// 如果读扇区时的出错次数大于等于7. 则结束请求并唤醒等待该请求的的进程。而且缓冲区头标志为没有更新
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	// 如果读扇区时的出错次数大于3次，则要求复位硬盘控制器操作。
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

/**
 *	读操作中断调用函数。
 *  将在执行硬盘中断处理程序中调用
 */
static void read_intr(void)
{
	if (win_result()) {		// 如果控制器忙，读写错误或者命令执行错误
		bad_rw_intr();		// 调用硬盘读写失败函数
		do_hd_request();	// 然后再次请求硬盘做相应处理（复位）
		return;
	}
	port_read(HD_DATA,CURRENT->buffer,256);	// 将数据从数据寄存器口读到请求结构缓冲中
	CURRENT->errors = 0;					// 清出错次数
	CURRENT->buffer += 512;					// 调整缓冲区指针，指向新的空区域
	CURRENT->sector++;						// 起始扇区号加1
	if (--CURRENT->nr_sectors) {			// 如果仍然有要读取的扇区
		do_hd = &read_intr;					// 再次置硬盘调用c函数指针位read_intr
		return;
	}
	end_request(1);				// 请求结束，并将缓冲区标志置为1.表示有更新
	do_hd_request();						// 执行其他硬盘请求操作
}

/**
 * 写扇区中断调用函数。在硬盘中断处理程序中被调用
 * 在写命令执行后，会产生硬盘中断信号，执行硬盘中断处理程序。此时在硬盘中断处理程序中调用的c函数指针do_hd已经
 * 执行write_intr。 因此会在写操作完成（或出错）后，执行该函数
 */
static void write_intr(void)
{
	if (win_result()) {		// 如果控制器忙，读写错误或者命令执行错误
		bad_rw_intr();		// 调用硬盘读写失败函数
		do_hd_request();	// 然后再次请求硬盘做相应处理（复位）
		return;
	}
	if (--CURRENT->nr_sectors) {	// 将写扇区数减1，若还有扇区要写
		CURRENT->sector++;				// 当前扇区号+1
		CURRENT->buffer += 512;			// 调整请求缓冲区指针
		do_hd = &write_intr;			// 将硬盘中断程序调用指针设置为write_intr
		port_write(HD_DATA,CURRENT->buffer,256);	// 再次向数据端口写入256字
		return;
	}
	end_request(1);		// 请求结束，并将缓冲区标志置为1，表示有更新
	do_hd_request();				// 执行其他硬盘请求操作
}

/**
 * 重新校正中用到的中断调用函数
 */
static void recal_intr(void)
{
	if (win_result())	// 如果硬盘控制器返回错误信息，则首先进行读写失败处理
		bad_rw_intr();	// 
	do_hd_request();	// 做复位处理。
}

/**
 * 执行硬盘读写请求操作
 */
void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;

	INIT_REQUEST;				// 检查请求的合法性。 kernel/blk_drv/blk.h。 校验了请求，主设备号，缓冲区头锁定状态
	dev = MINOR(CURRENT->dev);		// 取设备号中的子设备号。就是硬盘的分区号
	block = CURRENT->sector;		// 请求的起始扇区

	// 如果子设备号不存在或者大于该分区扇区数-2。 结束该请求，并跳转到repeat中继续执行。
	// 因此一次要求读写2个扇区（512*2B），所以请求的扇区号不能大于分区中倒数第二个扇区号
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
		end_request(0);
		goto repeat;
	}
	block += hd[dev].start_sect;	// 将所需读的块对应到整个硬盘上的绝对扇区号
	dev /= 5;						// 此时dev代表硬盘号（0或1）

	// 从硬盘信息结构中根据起始扇区号和每磁道扇区数计算在磁道中的扇区号sec，柱面号cyl，磁头号head。
	// 只有输出，没有输入，也没有指定需要修改的寄存器。
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info[dev].sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info[dev].head));
	sec++;							// 扇区号增加1
	nsect = CURRENT->nr_sectors;	// 读取的扇区数
	if (reset) {					// 重置标志
		reset = 0;
		recalibrate = 1;			// 需要重新校正
		reset_hd(CURRENT_DEV);	// 重置磁盘
		return;
	}

	// 校正
	if (recalibrate) {				
		recalibrate = 0;		// 复位校正位
		// 向硬盘控制器发送重新校正信号
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	

	// 如果是血扇区操作
	if (CURRENT->cmd == WRITE) {
		// 发送写命令，循环读取状态寄存器信息并判断请求服务标志DRQ_STAT是否置为。
		// DRQ_STAT数硬盘状态寄存器请求服务位 include/linux/hdreg.h
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		// 如果硬盘请求服务置位，那么就退出循环。
		// 如果等到循环结束还没有置位，标志这次写操作失败。处理下一个磁盘请求。
		if (!r) {
			bad_rw_intr();
			goto repeat;
		}
		// 向硬盘控制器数据寄存器端口HD_DATA 写入1个扇区的数据
		port_write(HD_DATA,CURRENT->buffer,256);
	} else if (CURRENT->cmd == READ) {
		// 读武器请求，则向硬盘控制器发送读请求。
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
	} else
		panic("unknown hd-command");
}

/**
 * 磁盘初始化函数
 */
void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST; // 设置代理函数
	set_intr_gate(0x2E,&hd_interrupt);	// 设置磁盘中断初始化向量
	outb_p(inb_p(0x21)&0xfb,0x21);		// 读取0x21的数据，并写入到0x21中
	outb(inb_p(0xA1)&0xbf,0xA1);		// 读取0xA1的数据，并写入到0xA1中
}
