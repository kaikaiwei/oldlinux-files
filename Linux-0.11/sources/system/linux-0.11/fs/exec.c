/*
 *  linux/fs/exec.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * #!-checking implemented by tytso.
 */

/*
 * Demand-loading implemented 01.12.91 - no need to read anything but
 * the header into memory. The inode of the executable is put into
 * "current->executable", and page faults do the actual loading. Clean.
 *
 * Once more I can proudly say that linux stood up to being changed: it
 * was less than 2 hours work to get demand-loading completely implemented.
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <a.out.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/segment.h>

// 程序退出系统调用 exit.c中
extern int sys_exit(int exit_code);
// 文件关闭系统调用 file.c中
extern int sys_close(int fd);

/*
 * MAX_ARG_PAGES defines the number of pages allocated for arguments
 * and envelope for the new program. 32 should suffice, this gives
 * a maximum env+arg of 128kB !
 * 定义了给新程序分配 参数和环境变量所使用的内存最大页数。 32 * 4KB = 128KB
 */
#define MAX_ARG_PAGES 32

/*
 * create_tables() parses the env- and arg-strings in new user
 * memory and creates the pointer tables from them, and puts their
 * addresses on the "stack", returning the new stack pointer value.
 * 在新用户堆栈汇总闯将环境和参数变量指针表。
 * @param p 数据段末尾（高端地址）
 * @param argc 参数个数
 * @param envc 环境变量个数
 */
static unsigned long * create_tables(char * p,int argc,int envc)
{
	unsigned long *argv,*envp;
	unsigned long * sp;

	// 堆栈指针对齐到4字节
	sp = (unsigned long *) (0xfffffffc & (unsigned long) p);
	sp -= envc+1;	// 堆栈指针向下移动环境参数个字节
	envp = sp;		// envp 指向sp
	sp -= argc+1;	// 堆栈指针向下移动命令行参数个空洞
	argv = sp;		// argc 指向sp
	// 保存参数入栈
	put_fs_long((unsigned long)envp,--sp);	// 保存envp指针到sp中
	put_fs_long((unsigned long)argv,--sp);	// 保存argv指针到sp中
	put_fs_long((unsigned long)argc,--sp);	// 保存argc指针到sp中

	// 将命令行参数指针放入空出来的相应地方，最后放NULL
	while (argc-->0) {
		put_fs_long((unsigned long) p,argv++);
		while (get_fs_byte(p++)) /* nothing */ ;
	}
	put_fs_long(0,argv);

	// 将命令行输出指针放入空出来的相应地方，最后放NULL
	while (envc-->0) {
		put_fs_long((unsigned long) p,envp++);
		while (get_fs_byte(p++)) /* nothing */ ;
	}
	put_fs_long(0,envp);
	return sp;
}

/*
 * count() counts the number of arguments/envelopes
 * 对指定参数数据进行进行统计
 * @param argv 参数数组
 * @return 参数个数
 */
static int count(char ** argv)
{
	int i=0;
	char ** tmp;

	if (tmp = argv)
		while (get_fs_long((unsigned long *) (tmp++)))
			i++;

	return i;
}

/*
 * 'copy_string()' copies argument/envelope strings from user
 * memory to free pages in kernel mem. These are in a format ready
 * to be put directly into the top of new user memory.
 *
 * Modified by TYT, 11/24/91 to add the from_kmem argument, which specifies
 * whether the string and the string array are from user or kernel segments:
 * 
 * from_kmem     argv *        argv **
 *    0          user space    user space
 *    1          kernel space  user space
 *    2          kernel space  kernel space
 * 
 * We do this by playing games with the fs segment register.  Since it
 * it is expensive to load a segment register, we try to avoid calling
 * set_fs() unless we absolutely have to.
 * 拷贝字符。 从用户空间复制参数和环境字符串到内核空闲页面内存中。
 * 这些已具有直接放到新用户内存中的格式
 * @param argc 参数个数
 * @param argv 参数数组
 * @param page 参数和环境页面指针
 * @param p 在参数表空间中的指针，始终指向已经复制的字符串头部
 * @param from_kmem 参数。 见上方注释
 */
static unsigned long copy_strings(int argc,char ** argv,unsigned long *page,
		unsigned long p, int from_kmem)
{
	char *tmp, *pag;
	int len, offset = 0;
	unsigned long old_fs, new_fs;

	// 已经复制的字符串头部为空，返回
	if (!p)
		return 0;	/* bullet-proofing */

	// 获取ds和es两个寄存器的值
	new_fs = get_ds();
	old_fs = get_fs();

	// 如果是都在内核空间，设置fs为内核空间寄存器的值
	if (from_kmem==2)
		set_fs(new_fs);

	// 字符拷贝
	while (argc-- > 0) {
		// 字符串在用户空间，字符串数组在内核空间
		if (from_kmem == 1)
			set_fs(new_fs);
		// 取fs中最后一个参数指针
		if (!(tmp = (char *)get_fs_long(((unsigned long *)argv)+argc)))
			panic("argc is wrong");
		// 字符串在用户空间，字符串数组载内核空间
		if (from_kmem == 1)
			set_fs(old_fs);
		// 长度为0， tmp会指向字符串的末端
		len=0;		/* remember zero-padding */
		do {
			len++;
		} while (get_fs_byte(tmp++));

		// 如果使用完了着128KB的空间
		if (p-len < 0) {	/* this shouldn't happen - 128kB */
			set_fs(old_fs);
			return 0;
		}

		// 字符拷贝， 逆向开始拷贝的
		while (len) {
			--p; --tmp; --len;
			// offset 初始化为0， 表示这里是首次复制。
			if (--offset < 0) {
				offset = p % PAGE_SIZE;
				if (from_kmem==2)
					set_fs(old_fs);
				// 新页面申请
				if (!(pag = (char *) page[p/PAGE_SIZE]) &&
				    !(pag = (char *) page[p/PAGE_SIZE] =
				      (unsigned long *) get_free_page())) 
					return 0;
				if (from_kmem==2)
					set_fs(new_fs);

			}
			// 复制到的位置
			*(pag + offset) = get_fs_byte(tmp);
		}
	}
	if (from_kmem==2)
		set_fs(old_fs);
	return p;
}

/**
 * 修改局部描述符表中的基址和段限长
 * @param text_size 执行头部文件中，a_text指出的代码段长度值
 * @param page 参数和环境空间页面指针
 * @return 数据段限长值
 */
static unsigned long change_ldt(unsigned long text_size,unsigned long * page)
{
	unsigned long code_limit,data_limit,code_base,data_base;
	int i;

	code_limit = text_size+PAGE_SIZE -1;
	code_limit &= 0xFFFFF000;	// 对齐长度。  以页面为单位的代码段限制长度
	data_limit = 0x4000000;		// 数据段长度为64MB

	// 拿到当前数据段的基址
	code_base = get_base(current->ldt[1]);
	data_base = code_base;	//代码段机制和数据段基址相同

	// 设置代码段基址
	set_base(current->ldt[1],code_base);
	// 设置代码段长度
	set_limit(current->ldt[1],code_limit);
	// 设置数据段基址
	set_base(current->ldt[2],data_base);
	// 设置数据段限长
	set_limit(current->ldt[2],data_limit);

	/* make sure fs points to the NEW data segment */
	// 保证fs指向新的数据段
	// fs放入局部表数据段描述符， 0x17
	__asm__("pushl $0x17\n\tpop %%fs"::);

	// data_base指向数据段末尾
	data_base += data_limit;
	// 将参数和环境变量存入的页面，放入到数据段线性地址末端。
	for (i=MAX_ARG_PAGES-1 ; i>=0 ; i--) {
		data_base -= PAGE_SIZE;
		if (page[i])	// 如果页面存在
			put_page(page[i],data_base);	// 放在用户空间指定地址上
	}
	return data_limit;
}

/*
 * 'do_execve()' executes a new program.
 * 加载一个新的程序执行
 * @param eip 堆栈中，系统调用中断程序代码指针
 * @param tmp	系统调用中断代码的返回地址
 * @param filename 执行文件路径名称
 * @param argv	参数数组
 * @param envp	环境数组
 * @return 
 */
int do_execve(unsigned long * eip,long tmp,char * filename,
	char ** argv, char ** envp)
{
	struct m_inode * inode;	// 文件inode
	struct buffer_head * bh;	// 高速缓冲区
	struct exec ex;				// 执行文件头部数据缓冲变量
	unsigned long page[MAX_ARG_PAGES];	// 进程内存分布
	int i,argc,envc;			// 参数和环境字符床空间的页面指针数组
	int e_uid, e_gid;			// 有效用户id和有效组id
	int retval;					// 返回值
	int sh_bang = 0;			/// 控制是否需要执行脚本处理代码
	unsigned long p=PAGE_SIZE*MAX_ARG_PAGES-4;	// 参数和环境字符串空间中的偏移指针，初始化为指向该空间的最后一个长字处。

	// eip[1] 中是原代码段寄存器cs， 其中的选择符不可以是内核段选择父，即内核不能调用本程序
	if ((0xffff & eip[1]) != 0x000f)
		panic("execve called from supervisor mode");

	// 初始化参数和环境串空间的页面指针数组（表）
	for (i=0 ; i<MAX_ARG_PAGES ; i++)	/* clear page-table */
		page[i]=0;

	// 获取可执行文件的inode
	if (!(inode=namei(filename)))		/* get executables inode */
		return -ENOENT;

	// 获取参数个数
	argc = count(argv);
	// 获取环境变量个数
	envc = count(envp);
	

// 
restart_interp:
	// 执行文件必须是常规文件，否则跳转到exec_error2处执行
	if (!S_ISREG(inode->i_mode)) {	/* must be regular file */
		retval = -EACCES;
		goto exec_error2;
	}
	i = inode->i_mode;	// 获取文件模式
	// 检查文件的执行权限，根据其属性，查看本程序是否可执行
	e_uid = (i & S_ISUID) ? inode->i_uid : current->euid;
	e_gid = (i & S_ISGID) ? inode->i_gid : current->egid;
	if (current->euid == inode->i_uid)	// 是所属用户
		i >>= 6;
	else if (current->egid == inode->i_gid)	// 是同组用户
		i >>= 3;
	// 没有可执行或者 文件不可执行的超级用户执行
	if (!(i & 1) &&
	    !((inode->i_mode & 0111) && suser())) {
		retval = -ENOEXEC;
		goto exec_error2;
	}

	// 读取可执行文件的第一块数据到高速缓冲区。 出错则返回错误号
	if (!(bh = bread(inode->i_dev,inode->i_zone[0]))) {
		retval = -EACCES;
		goto exec_error2;
	}

	// 将数据转换位struct exec的结构体。 
	ex = *((struct exec *) bh->b_data);	/* read exec-header */
	// 如果是以#！开头，并且sh_bang没有置位。 处理脚本文件执行
	if ((bh->b_data[0] == '#') && (bh->b_data[1] == '!') && (!sh_bang)) {
		/*
		 * This section does the #! interpretation.
		 * Sorta complicated, but hopefully it will work.  -TYT
		 */

		char buf[1023], *cp, *interp, *i_name, *i_arg;
		unsigned long old_fs;

		// 复制执行程序头一行后面的字符串到buf中，其中还有脚本处理程序名
		strncpy(buf, bh->b_data+2, 1022);
		brelse(bh);		// 释放缓冲区
		iput(inode);	// 释放inode
		buf[1022] = '\0';	
		// 取第一行内容，并删除开始的空格，制表符
		if (cp = strchr(buf, '\n')) {
			*cp = '\0';
			for (cp = buf; (*cp == ' ') || (*cp == '\t'); cp++);
		}

		// 如果这个时候cp不合法，cp=0或者等于\0
		if (!cp || *cp == '\0') {
			retval = -ENOEXEC; /* No interpreter name found */
			goto exec_error1;
		}

		// 记录程序名称
		interp = i_name = cp;
		// 首先是取第一个字符串，时脚本解释程序名，iname指向它
		i_arg = 0;
		for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++) {
 			if (*cp == '/')
				i_name = cp+1;
		}
		// 如果cp后面还有字符串， 那么就是参数串，让i_arg指向它
		if (*cp) {
			*cp++ = '\0';
			i_arg = cp;
		}
		/*
		 * OK, we've parsed out the interpreter name and
		 * (optional) argument.
		 * 我们已经解析了文件名和可选的参数
		 */
		 // 如果sh_bang参数没有设置，就设置它
		if (sh_bang++ == 0) {
			// 拷贝是指定个数的环境变量串和环境变量串
			p = copy_strings(envc, envp, page, p, 0);
			p = copy_strings(--argc, argv+1, page, p, 0);
		}
		/*
		 * Splice in (1) the interpreter's name for argv[0]
		 *           (2) (optional) argument to interpreter
		 *           (3) filename of shell script
		 *
		 * This is done in reverse order, because of how the
		 * user environment and arguments are stored.
		 * 拼接 1: argv[0]中放解释程序的名称
		 *     2:解释程序参数
		 *     3: 脚本程序的名称
		 * 这是以逆序进行处理的，是由用户环境和参数的存放方式造成的
		 */
		 // 复制脚本程序文件名到参数和环境空间中
		p = copy_strings(1, &filename, page, p, 1);
		argc++;
		// 如果有参数， 拷贝参数
		if (i_arg) {
			p = copy_strings(1, &i_arg, page, p, 2);
			argc++;
		}
		// 复制解释程序文件名到参数和环境空间中
		p = copy_strings(1, &i_name, page, p, 2);
		argc++;
		if (!p) {
			retval = -ENOMEM;
			goto exec_error1;
		}

		/*
		 * OK, now restart the process with the interpreter's inode.
		 * 显示使用解释程序的i节点重起进程
		 */
		old_fs = get_fs();
		set_fs(get_ds());
		// 获取到和执行文件的头节点
		if (!(inode=namei(interp))) { /* get executables inode */
			set_fs(old_fs);
			retval = -ENOENT;
			goto exec_error1;
		}
		set_fs(old_fs);
		goto restart_interp;// 重新开始执行
	}

	// 释放头节点
	brelse(bh);

	// 对执行头文件进行处理
	// 如果执行文件不是需求也可执行文件。 或者代码重定位长度（a_trsize）不等与0
	// 或者数据重定位信息长度不等于0， 或者代码段+数据段+堆栈长度超过50MB， 
	// 或者i节点表明的该执行文件长度小于代码段+数据段+符号表长度+执行头部长度的总和
	if (N_MAGIC(ex) != ZMAGIC || ex.a_trsize || ex.a_drsize ||
		ex.a_text+ex.a_data+ex.a_bss>0x3000000 ||
		inode->i_size < ex.a_text+ex.a_data+ex.a_syms+N_TXTOFF(ex)) {
		retval = -ENOEXEC;
		goto exec_error2;
	}

	// 如果执行文件头部长度不等于一个内存快大小（1024B）， 也不能执行 
	if (N_TXTOFF(ex) != BLOCK_SIZE) {
		printk("%s: N_TXTOFF != BLOCK_SIZE. See a.out.h.", filename);
		retval = -ENOEXEC;
		goto exec_error2;
	}

	// 如果sh_bang没有置位，则复制指定个数的环境变量字符串和参数到参数和环境空间中
	if (!sh_bang) {
		p = copy_strings(envc,envp,page,p,0);
		p = copy_strings(argc,argv,page,p,0);
		if (!p) {
			retval = -ENOMEM;
			goto exec_error2;
		}
	}

	/* OK, This is the point of no return */ // 从下面开始就没有返回的地方了
	// 如果原程序也是一个执行文件，释放其i节点，并让程序executable字段指向新进程i节点
	if (current->executable)
		iput(current->executable);
	current->executable = inode;

	// 复位所有信号处理句柄。 但是SIG_IGN句柄不能复位， 因此在
	for (i=0 ; i<32 ; i++)
		current->sigaction[i].sa_handler = NULL;
	// 关闭所有打开的文件， 并置位close_on_exec
	for (i=0 ; i<NR_OPEN ; i++)
		if ((current->close_on_exec>>i)&1)
			sys_close(i);
	current->close_on_exec = 0;

	// 根据指定的地址和限长，释放原程序代码段和数据段对应的内存页表指定的内存块及页表本身
	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));
	// 如果使用了数字协处理器，则将last_task_used_math置空
	if (last_task_used_math == current)
		last_task_used_math = NULL;
	current->used_math = 0;

	// 根据a_text修改局部表中描述符基址和段限长，并将参数和环境空间页面防止载数据段末端。
	// 执行下面语句后，p此时时以数据段起始处为原点的偏移量，仍指向参数和环境空间数据开始处，
	// 也就是，转换为堆栈的指针
	p += change_ldt(ex.a_text,page)-MAX_ARG_PAGES*PAGE_SIZE;
	p = (unsigned long) create_tables((char *)p,argc,envc); // 在新用户堆栈中创建环境和参数变量指针表，并返回该堆栈
	// 修改当前进程个字段位新执行程序的信息。
	// 代码段尾值（current->end_code）等于a_text
	// 数据段尾值（current->end_data）等于a_data
	// 进程结尾字段（brk） = a_bss + 代码段尾值+ 数据段尾值
	current->brk = ex.a_bss +
		(current->end_data = ex.a_data +
		(current->end_code = ex.a_text));
	// 堆栈指针。
	current->start_stack = p & 0xfffff000;
	// 进程uid和gid
	current->euid = e_uid;
	current->egid = e_gid;
	// 初始化一页bss数据，全为0
	i = ex.a_text+ex.a_data;
	while (i&0xfff)
		put_fs_byte(0,(char *) (i++));
	// 将中出处理函数的代码指针替换为新的代码指针
	eip[0] = ex.a_entry;		/* eip, magic happens :-) */
	eip[3] = p;			/* stack pointer */  // esp 堆栈指针
	return 0;			// 返回数据将弹出这些堆栈数据，并使cpu执行新的进程
exec_error2:
	iput(inode);	// 释放i节点
exec_error1:
	// 释放页面信息， 参数和环境变量空间
	for (i=0 ; i<MAX_ARG_PAGES ; i++)
		free_page(page[i]);
	return(retval);
}
