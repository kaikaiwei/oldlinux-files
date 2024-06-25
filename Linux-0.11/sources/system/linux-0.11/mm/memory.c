/*
 *  linux/mm/memory.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * demand-loading started 01.12.91 - seems it is high on the list of
 * things wanted, and it should be easy to implement. - Linus
 */

/*
 * Ok, demand-loading was easy, shared pages a little bit tricker. Shared
 * pages started 02.12.91, seems to work. - Linus.
 *
 * Tested sharing by executing about 30 /bin/sh: under the old kernel it
 * would have taken more than the 6M I have free, but it worked well as
 * far as I could see.
 *
 * Also corrected some "invalidate()"s - I wasn't doing enough of them.
 */

/**
 *  实现了对主内存区内存的动态分配和收回操作。对于物理内存的管理，使用字节数组（mem_map）来表示主内存区中所有物理内存页的状态。
 *  每个字节描述一个物理页的占用状态。0表示对应的物理内存空闲。当申请一页物理内存时，就将对应的字节的值增1.
 *  
 *  每个进程的线性地址中都是从nr*64MB的地址位置开始（nr是任务号），占用线性地址空间的范围是64MB。
 *  其中最后部分的环境参数数据块最长位128KB， 其左面的起始时堆栈指针。在进程创建时，bss段的第一页被初始化为全0.
 *  布局是这样的：代码段（nr*64MB开始），数据段，bss段， 。。。， （堆栈指针），参数（环境参数块，128KB）
 */
#include <signal.h>

#include <asm/system.h>

#include <linux/sched.h>
#include <linux/head.h>			// 定义了段描述符
#include <linux/kernel.h>		// 内核常用函数的原型定义

/** 推出进程的操作*/
volatile void do_exit(long code);	// kernel/exit.c中

/** out of memory。 内存溢出。会调用do_exit函数，并推出*/
// 显示内存已经用完的信息
static inline volatile void oom(void)
{
	printk("out of memory\n\r");
	do_exit(SIGSEGV);
}

/** 
 * 刷新页变换告诉缓冲区的宏
 * 为了提高地址转换的效率，cpu将最近使用的页表数据存放在芯片中高速缓冲中。
 * 在修改过页表信息后，就要冲新刷新该缓冲区。
 * 这里使用重新加载 页目录基地址 cr3 的方法进行刷新。eax=0，数页目录的基址
 * invalidate 宏， movl，高字节移动。将eax高16位移动到cr3中。
 * eax=0， 是页目录的基址
 */
#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

/* these are not to be changed without changing head.s etc */
// 低端内存地址（1MB）
#define LOW_MEM 0x100000
// 分页内存15MB。主内存区域中最多15MB
#define PAGING_MEMORY (15*1024*1024)
// 分页后的物理内存页数。
#define PAGING_PAGES (PAGING_MEMORY>>12)
// 执行内存地址映射为页号
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)
// 页面被占用标志。
#define USED 100

/**
 * 用于判断给定地址是否位于当前进程的代码段中。
 */
#define CODE_SPACE(addr) ((((addr)+4095)&~4095) < \
current->start_code + current->end_code)

static long HIGH_MEMORY = 0;	// 高端内存地址。实际物理内存的最高端地址

/* 
 * 拷贝内存的宏操作。复制一页内存宏
 * cld 方向位。rep，循环。 执行指令movsl。
 * S，来源地址
 * D，目的地址
 * c： 1024个
 * 会修改cx，di，si。
 * movsl 复制4个字节。 
 * MOVSB：传送单一字节
 * MOVSW：传送一个字（2字节）
 * MOVSL：传送一个双字（4字节）
 */
#define copy_page(from,to) \
__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024):"cx","di","si")

/** 
 * 内存映射字节图（1B表示1页）。每个页面对应的字节用于标记页面当前被引用的次数
 * 主内存区所有页面的状态
 */
static unsigned char mem_map [ PAGING_PAGES ] = {0,};

/*
 * Get physical address of first (actually last :-) free page, and mark it
 * used. If no free pages left, return 0.
 * 在主内存区中申请一页空闲内存页，并返回物理内存页的起始地址（标记为使用中）。
 * 从后面开始往前进行搜索
 * 搜线性扫描内存页面字节数组mem_map，寻找值为0的字节项。
 * 输入 %1（ax=0）-0， %2（LOW_MEM), %3(cx=PAGING_PAGES), %4(edi=mem_map+PAGING_PAGES-1)
 */
unsigned long get_free_page(void)
{
register unsigned long __res asm("ax"); // 使用ax寄存器作为返回地址。

// 嵌入式汇编
__asm__("std ; repne ; scasb\n\t"		// 置方向位。如果不为0，就继续。将al（0）与对应的每个页面（di）的内容比较
	"jne 1f\n\t"						// 如果没有等于0的字节，那么跳转到1:，程序结束，返回0
	"movb $1,1(%%edi)\n\t"				// 将对应页面的内存映像位置1
	"sall $12,%%ecx\n\t"				// 页面数*4KB=相对页面起始地址
	"addl %2,%%ecx\n\t"					// 再加上低端内存地址，即可获得实际物理起始地址
	"movl %%ecx,%%edx\n\t"				// 将页面实际起始地址放到edx寄存器中
	"movl $1024,%%ecx\n\t"				// exc=1024
	"leal 4092(%%edx),%%edi\n\t"		// edx = 4092+edx，表示该页的末端
	"rep ; stosl\n\t"					// 将edx指向的页面清0. 重复执行，stosl，
	"movl %%edx,%%eax\n"				// eax=edx， eax作为页面起始地址，返回
	"1:"
	:"=a" (__res)
	:"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES),"D" (mem_map+PAGING_PAGES-1)
	:"di","cx","dx");
return __res;
}

/*
 * Free a page of memory at physical address 'addr'. Used by
 * 'free_page_tables()'
 * 释放指定地址处的一页物理内存。 首先判断指定内存地址是否小于1MB。1MB以下的是内核专用的，返回0.
 * 如果超出
 */
void free_page(unsigned long addr)
{
	if (addr < LOW_MEM) return; // 1MB以下的内存地址，内核专用。直接返回。
	// 超出物理内存最大地址，显示出错信息
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	// 进行页面号的换算。（addr-low_mem）/4KB.
	addr -= LOW_MEM;
	addr >>= 12;
	// 减少引用计数
	if (mem_map[addr]--) return;
	// 多次释放，清零，并打印错误。
	mem_map[addr]=0;
	panic("trying to free free page");
}

/*
 * This function frees a continuos block of page tables, as needed
 * by 'exit()'. As does copy_page_tables(), this handles only 4Mb blocks.
 * 释放指定线性地址和长度（页表个数）对应的物理内存页块。
 * 不仅对管理线性地址的页目录和页表中的对应项内容进行修改，而且也对每个页表中所有页表项对应的物理内存页进行释放或占用操作。
 * @param from 线性地址
 * @param size 页表个数
 */
int free_page_tables(unsigned long from,unsigned long size)
{
	unsigned long *pg_table;
	unsigned long * dir, nr;

	if (from & 0x3fffff) // 载4MB的边界上
		panic("free_page_tables called with wrong alignment");
	// 若果为0， 就是在释放内核和缓冲区所占用的空间。
	if (!from)
		panic("Trying to free up swapper memory space");

	// 计算页目录表中所占用页目录项数，size，也就是页表个数。并计算启示目录项号。 4MB的整数倍
	size = (size + 0x3fffff) >> 22;
	// 计算起始目录项号
	dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */
	// 从起始目录项开始，释放所占用的所有size个目录项。同时释放页目录项所指的页表中的所有页表项和对应的物理内存页面。
	for ( ; size-->0 ; dir++) {
		if (!(1 & *dir))
			continue;
		// 同时释放页目录项所指的页表中的所有页表项和对应的物理内存页面。
		pg_table = (unsigned long *) (0xfffff000 & *dir);
		for (nr=0 ; nr<1024 ; nr++) {
			if (1 & *pg_table)
				free_page(0xfffff000 & *pg_table);
			*pg_table = 0;
			pg_table++;
		}
		free_page(0xfffff000 & *dir);
		*dir = 0;
	}
	invalidate(); // 将eax移动cr3中，cr3是用来做地址转换的。
	return 0;
}

/*
 *  Well, here is one of the most complicated functions in mm. It
 * copies a range of linerar addresses by copying only the pages.
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)
 *
 * Note! We don't copy just any chunks of memory - addresses have to
 * be divisible by 4Mb (one page-directory entry), as this makes the
 * function easier. It's used only by fork anyway.
 *
 * NOTE 2!! When from==0 we are copying kernel space for the first
 * fork(). Then we DONT want to copy a full page-directory entry, as
 * that would lead to some serious memory waste - we just copy the
 * first 160 pages - 640kB. Even that is more than we need, but it
 * doesn't take any more memory - we don't copy-on-write in the low
 * 1 Mb-range, so the pages can be shared with the kernel. Thus the
 * special case for nr=xxxx.
 * 复制执行线性地址和长度（页表个数）内存对应的页目录项和页表项，被复制的页目录表和页表对应的原物理内存区被共享使用。
 * @param from 线性地址
 * @param to 目的线性地址
 * @param size 页表个数
 */
int copy_page_tables(unsigned long from,unsigned long to,long size)
{
	unsigned long * from_page_table;
	unsigned long * to_page_table;
	unsigned long this_page;
	unsigned long * from_dir, * to_dir;
	unsigned long nr;

	// 验证指定的源线性地址和目的线性地址是否都在4MB的内存边界地址上，否则就显示出错信息，并死机
	if ((from&0x3fffff) || (to&0x3fffff))
		panic("copy_page_tables called with wrong alignment");
	// 换算出起始页目录项
	from_dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */
	to_dir = (unsigned long *) ((to>>20) & 0xffc);
	// 计算需要复制的内去取占用的页表数（页目录项数）， 最高10位
	size = ((unsigned) (size+0x3fffff)) >> 22;
	// 开始进行复制。
	for( ; size-->0 ; from_dir++,to_dir++) {
		if (1 & *to_dir)	// 如果页表已经存在
			panic("copy_page_tables: already exist");
		// 如果源目录项中页表没有在使用，跳过。
		if (!(1 & *from_dir))
			continue;
		// 取当前源目录项中页表地址
		from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
		// 为目的页表去一页空闲内存。如果返回0，则说明没有申请到空闲内存。
		if (!(to_page_table = (unsigned long *) get_free_page()))
			return -1;	/* Out of memory, see freeing */
		// 设置目的目录项的信息。7是标志信息，表示（Usr，R/W，Present）
		*to_dir = ((unsigned long) to_page_table) | 7;
		// 针对当前处理的页表，设置需复制的页面数。如果是在内核空间，则仅需要复制头160页。否则需要复制1个页表中的所有1024页表项
		nr = (from==0)?0xA0:1024;
		// 对于当前页表，开始复制指定数据nr个内存页表项
		for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {
			this_page = *from_page_table; // 源页表项内容
			// 如果源页表项没有在使用，则不用复制。跳过
			if (!(1 & this_page))
				continue;
			// 复制页表项中R/W标志（置0）
			this_page &= ~2;
			// 将该页表项复制到目的页表中
			*to_page_table = this_page;
			// 如果该页表所执行的地址在1MB以上，则需要设置内存映射数组mem_map。
			if (this_page > LOW_MEM) {
				*from_page_table = this_page;
				this_page -= LOW_MEM;	// 计算页面号
				this_page >>= 12;		
				mem_map[this_page]++; // mem_map中使用进行递增。
			}
		}
	}
	invalidate();	//刷新页变换高速缓冲
	return 0;
}

/*
 * This function puts a page in memory at the wanted address.
 * It returns the physical address of the page gotten, 0 if
 * out of memory (either when trying to access page-table or
 * page.)
 * 将指定的物理内存页映射到指定的线性地址中去。
 * @param page 物理内存页起始地址
 * @param address 线性地址
 * @return 返回物理页面的地址
 */
unsigned long put_page(unsigned long page,unsigned long address)
{
	unsigned long tmp, *page_table;

/* NOTE !!! This uses the fact that _pg_dir=0 */

	// 验证地址的合法性
	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n",page,address);
	// page对应的物理内存的P位（有效位）不为1
	if (mem_map[(page-LOW_MEM)>>12] != 1)
		printk("mem_map disagrees with %p at %p\n",page,address);
	// 得到页表地址
	page_table = (unsigned long *) ((address>>20) & 0xffc);
	if ((*page_table)&1) // p标记，1表示在使用中，否则需要申请新的内存页
		page_table = (unsigned long *) (0xfffff000 & *page_table);
	else {
		// 申请新的内存页
		if (!(tmp=get_free_page()))
			return 0;
		// 7是标志信息，表示（Usr，R/W，Present）
		*page_table = tmp|7;
		page_table = (unsigned long *) tmp;
	}
	page_table[(address>>12) & 0x3ff] = page | 7;
/* no need for invalidate */
	return page;
}

/**
 * 取消写保护函数。 用于中断异常中写保护处理
 * @param table_entry 页表项指针 Un Write-Protected
 */
void un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page,new_page;

	// 取页表项内的物理页面地址
	old_page = 0xfffff000 & *table_entry;
	// 如果物理页面地址超过低端内存， 并且转换为页号后，得到的物理内存页的引用计数为1，也就是页面没有被共享
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {
		*table_entry |= 2;	// 设置 R/W 标记
		invalidate();		// 刷新高速缓存区
		return;
	}
	// 获取新的页面，失败提示oom
	if (!(new_page=get_free_page()))
		oom();
	// 减少引用计数
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	// 7是标志信息，表示（Usr，R/W，Present）
	*table_entry = new_page | 7;
	invalidate();	// 刷新高速缓存
	// 拷贝旧的内存页内容到新的内容中去
	copy_page(old_page,new_page);
}	

/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * If it's in code space we exit with a segment error.
 * 处理写时复制。在共享页面的情况下。会取消对页面的共享。
 * 会先判断是否在进程的代码区域，如果是则终止程序（代码区域不能被该懂，然后执行写时复制页面的额操作
 */
void do_wp_page(unsigned long error_code,unsigned long address)
{
#if 0
/* we cannot do this yet: the estdio library writes to code space */
/* stupid, stupid. I really want the libc.a from GNU */
	// 如果是在代码段空间
	if (CODE_SPACE(address))
		do_exit(SIGSEGV);
#endif
	// 执行写时复制，去掉空间
	// 计算页表项指针 ((address>>10) & 0xffc
	// 计算指定的页面在页表中的偏移项 (0xfffff000 & *((unsigned long *) ((address>>20) &0xffc)
	un_wp_page((unsigned long *)
		(((address>>10) & 0xffc) + (0xfffff000 &
		*((unsigned long *) ((address>>20) &0xffc)))));

}

/** 
 * 写入地址验证
 * 如果页面不可写，则复制页面
 * 在fork.c中被调用
 * @param addr 
 */
void write_verify(unsigned long address)
{
	unsigned long page;

	// 判断指定地址所对应的页目录项的页表是否存在。 地址的页目录项的P标志为1，表示可写入
	if (!( (page = *((unsigned long *) ((address>>20) & 0xffc)) )&1))
		return;
	// 取页表的地址
	page &= 0xfffff000;
	// 然后加上在页面中的偏移量
	page += ((address>>10) & 0xffc);
	// 如果该页面不可写， 则执行共享和复制
	if ((3 & *(unsigned long *) page) == 1)  /* non-writeable, present */
		un_wp_page((unsigned long *) page);
	return;
}

/**
 * 用于获取一页空闲物理内存并映射到指定线性地址处。
 * 通过使用get_free_page和put_page函数来实现这个功能
 * @param address 线性地址
 */
void get_empty_page(unsigned long address)
{
	unsigned long tmp;

	if (!(tmp=get_free_page()) || !put_page(tmp,address)) {
		free_page(tmp);		/* 0 is ok - ignored */
		oom();
	}
}

/*
 * try_to_share() checks the page at address "address" in the task "p",
 * to see if it exists, and if it is clean. If so, share it with the current
 * task.
 *
 * NOTE! This assumes we have checked that p != current, and that they
 * share the same executable.
 * 尝试对进程指定地址处的页面进行共享
 * @param address 指定的地址
 * @param p 任务数据结构
 * return 1:成功， 0 失败
 */
static int try_to_share(unsigned long address, struct task_struct * p)
{
	unsigned long from;
	unsigned long to;
	unsigned long from_page;
	unsigned long to_page;
	unsigned long phys_addr;

	// 指定地址的页目录项
	from_page = to_page = ((address>>20) & 0xffc);
	// 加上页面中的偏移量， 页目录项
	from_page += ((p->start_code>>20) & 0xffc);

	// 当前进程代码段的页目录项
	to_page += ((current->start_code>>20) & 0xffc);

	// 是否存在页目录，起始就是页目录的P位是否为1
	/* is there a page-directory at from? */
	from = *(unsigned long *) from_page;
	if (!(from & 1))
		return 0;
	
	// 计算页表项指针
	from &= 0xfffff000;
	from_page = from + ((address>>10) & 0xffc);
	phys_addr = *(unsigned long *) from_page;

	// 0x41 对应的是Dirty 和Present标志，如果不是使用中或有更新过。返回0
	/* is the page clean and present? */
	if ((phys_addr & 0x41) != 0x01)
		return 0;

	// 得到物理地址
	phys_addr &= 0xfffff000;
	// 物理地址超出界限
	if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
		return 0;

	// 是否存在目标地址。不存在则获取新的页面。
	// 页目录项的内容到to中。
	to = *(unsigned long *) to_page;
	// 如果to页目录项没有P标志
	if (!(to & 1))
		if (to = get_free_page())
			*(unsigned long *) to_page = to | 7;
		else
			oom();
	// 页表地址保存在to中
	to &= 0xfffff000;
	// 页表项地址保存在to_page中
	to_page = to + ((address>>10) & 0xffc);
	if (1 & *(unsigned long *) to_page)
		panic("try_to_share: to_page already exists");

	// 设置写保护
/* share them: write-protect */
	*(unsigned long *) from_page &= ~2;
	*(unsigned long *) to_page = *(unsigned long *) from_page;
	// 刷新缓存
	invalidate();
	phys_addr -= LOW_MEM;	// 物理地址减去低端内存
	phys_addr >>= 12;			// 计算页面号
	mem_map[phys_addr]++;		// 对应页面好的引用计数+1
	return 1;
}

/*
 * share_page() tries to find a process that could share a page with
 * the current one. Address is the address of the wanted page relative
 * to the current data space.
 *
 * We first check if it is at all feasible by checking executable->i_count.
 * It should be >1 if there are other tasks sharing this inode.
 * 本函数试图找到一个进程，它可以与当前进程共享页面
 * @param address 当前数据空间中期望共享的某页面地址
 * 首先通过检测executable-》i_count来查证是否可行。如果已经有有任务共享该inode， 它应该大于1。 
 * @return 1:成功， 0:失败
 */
static int share_page(unsigned long address)
{
	struct task_struct ** p;

	// 当前是否可执行。 excutable是执行程序的i节点结构
	if (!current->executable)
		return 0;
	// 仅单独执行下，也退出
	if (current->executable->i_count < 2)
		return 0;
	// 搜索任务数组中所有任务，寻找于当前进程可共享页面的进程，并尝试共享
	for (p = &LAST_TASK ; p > &FIRST_TASK ; --p) {
		if (!*p)	// 进程咩有在使用中
			continue;
		if (current == *p)	// 跳过当前进程
			continue;
		// 跳过可执行节点不一样的进程
		if ((*p)->executable != current->executable)
			continue;
		if (try_to_share(address,*p))
			return 1;
	}
	return 0;
}

/** 
 * 将需要的页面从块设备中读取到内存指定位置。用于处理缺页异常。
 * 首先判断指定的线性地址在一个进程空间中相对于进程基址的偏移长度值。如果它大于代码加数据长度，
 * 或者进程刚开始创建，则立即申请一页物理内存，并映射到进程线性地址中，然后返回。
 * 接着尝试进行页面共享操作。若成功，则立即返回。
 * 否则申请一页内存并从设备中读如一页信息，若加入该页信息时，执行线性地址+1页长度超过了进程代码加数据的长度，则将超过的部分清零。
 * 然后将该页映射到指定的线性地址处。
 * page.s中被调用
 */
void do_no_page(unsigned long error_code,unsigned long address)
{
	int nr[4];
	unsigned long tmp;
	unsigned long page;
	int block,i;

	// 页面的线性地址
	address &= 0xfffff000;
	// 线性地址在进程个空间中相对于进程基址的偏移长度
	tmp = address - current->start_code;
	// 如果当前进程的executable为空或者想读偏移超出代码+数据段长度， 则申请一页物理内存，并映射到线性地址处
	if (!current->executable || tmp >= current->end_data) {
		get_empty_page(address);
		return;
	}
	// 尝试共享页面成功，则退出
	if (share_page(tmp))
		return;
	// 取空闲页面，如果内存不够，显示内存不够，终止进程
	if (!(page = get_free_page()))
		oom();

	/* remember that 1 block is used for header */	// 记住，程序头要使用一个数据块
	block = 1 + tmp/BLOCK_SIZE; // 计算却也所在的数据块项
	// 根据i节点信息，取数据块在设备上对应的逻辑块号
	for (i=0 ; i<4 ; block++,i++)
		nr[i] = bmap(current->executable,block);
	// 读设备上一个页面的数据
	bread_page(page,current->executable->i_dev,nr);
	// 处理end_data， 在增加了一页内存后，该页内存可能超过end_data的区域。
	i = tmp + 4096 - current->end_data;
	// 对超出部分进行清零操作
	tmp = page + 4096;
	while (i-- > 0) {
		tmp--;
		*(char *)tmp = 0;
	}
	// 将物理页面放入到指定的线性空间中
	if (put_page(page,address))
		return;
	free_page(page);
	oom();
}

/** 
 * 内存初始化函数。 
 * @param start_mem 开始的内存地址
 * @param end_mem 内存的结束地址。
 */
void mem_init(long start_mem, long end_mem)
{
	int i;

	HIGH_MEMORY = end_mem; // 高端内存为内存的结束地址。
	// 对于每一个pages，标记为使用中。
	for (i=0 ; i<PAGING_PAGES ; i++)
		mem_map[i] = USED;		// 标记使用中
	i = MAP_NR(start_mem);		// 计算可使用起始内存的页面号
	end_mem -= start_mem;		// 再计算可分页处理的内存块大小
	end_mem >>= 12;				// 计算可用于分页的页面数
	while (end_mem-->0)			// 将所有可用于分页的页面映射数组清0，表示没有在使用中。
		mem_map[i++]=0;
}

/** 
 * 计算内存空闲页面数并显示
 */
void calc_mem(void)
{
	int i,j,k,free=0;
	long * pg_tbl;

	// 扫描mem_map， 获取空闲页面并显示
	for(i=0 ; i<PAGING_PAGES ; i++)
		if (!mem_map[i]) free++;
	printk("%d pages free (of %d)\n\r",free,PAGING_PAGES);
	// 扫描所有页目录项（排除0，1），如果所有页目录项有效，则统计对应页表中有效页面数，并显示
	for(i=2 ; i<1024 ; i++) {
		if (1&pg_dir[i]) {
			pg_tbl=(long *) (0xfffff000 & pg_dir[i]);
			for(j=k=0 ; j<1024 ; j++)
				if (pg_tbl[j]&1)
					k++;
			printk("Pg-dir[%d] uses %d pages\n",i,k);
		}
	}
}
