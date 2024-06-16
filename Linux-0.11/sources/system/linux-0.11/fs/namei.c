/*
 *  linux/fs/namei.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * Some corrections by tytso.
 * 根据目录名或文件名寻找对应i节点的函数。
 * 目录的建立和删除，目录项的建立和删除等操作函数和系统调用
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <const.h>
#include <sys/stat.h>

#define ACC_MODE(x) ("\004\002\006\377"[(x)&O_ACCMODE])

/*
 * comment out this line if you want names > NAME_LEN chars to be
 * truncated. Else they will be disallowed.
 */
/* #define NO_TRUNCATE */

#define MAY_EXEC 1		// 可执行（可进入）
#define MAY_WRITE 2		// 可写
#define MAY_READ 4		// 可读

/*
 *	permission()
 *
 * is used to check for read/write/execute permissions on a file.
 * I don't know if we should look at just the euid or both euid and
 * uid, but that should be easily changed.
 * 检查读写权限
 * @param inode 文件对应的i节点
 * @param mask 访问属性屏蔽码。
 * @return 1:许可访问，0:进制访问
 */
static int permission(struct m_inode * inode,int mask)
{
	int mode = inode->i_mode; // 文件访问属性

/* special case: not even root can read/write a deleted file */
	// 如果i节点有对应的设备，即使超级用户也不能访问
	if (inode->i_dev && !inode->i_nlinks)
		return 0;
	// 进程有效用户id等于i节点的用户id，则取文件宿主的访问权限
	else if (current->euid==inode->i_uid)
		mode >>= 6;
	// 进程有效组id等于i节点的用户id，则取文件组用户的访问权限
	else if (current->egid==inode->i_gid)
		mode >>= 3;
	// 如果上面所取得访问权限和平屏蔽码都相同，或者是超级用户，返回1
	if (((mode & mask & 0007) == mask) || suser())
		return 1;
	return 0;
}

/*
 * ok, we cannot use strncmp, as the name is not in our data space.
 * Thus we'll have to use match. No big problem. Match also makes
 * some sanity tests.
 *
 * NOTE! unlike strncmp, match returns 1 for success, 0 for failure.
 * 不使用strncmp匹配字符，因为名称不在内核空间中。
 * 所以使用match。 成功时返回1，失败时返回0
 * 指定铲毒字符串比较函数。
 * @param len 比较字符串的长度
 * @param name 文静名指针
 * @param de 目录项结构
 */
static int match(int len,const char * name,struct dir_entry * de)
{
	register int same __asm__("ax");

	// 检查目录项结构
	if (!de || !de->inode || len > NAME_LEN)
		return 0;
	if (len < NAME_LEN && de->name[len])
		return 0;
	// 使用嵌入式汇编进行比较
	__asm__("cld\n\t"
		"fs ; repe ; cmpsb\n\t"
		"setz %%al"
		:"=a" (same)
		:"0" (0),"S" ((long) name),"D" ((long) de->name),"c" (len)
		:"cx","di","si");
	return same;
}

/*
 *	find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 *
 * This also takes care of the few special cases due to '..'-traversal
 * over a pseudo-root and a mount point.
 * 在指定目录中寻找一个于名称匹配的目录项。 返回一个含有找到目录项的高速缓冲区即目录本身，作为参数res_dir
 * @param dir 目录项
 * @param name 名称
 * @param namelen 名称长度
 * @param res_dir 返回参数，用来返回目录项本身
 * @return buffer_head 还有目录项的高速缓冲区
 */
static struct buffer_head * find_entry(struct m_inode ** dir,
	const char * name, int namelen, struct dir_entry ** res_dir)
{
	int entries;
	int block,i;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;

	// 如果定义NO_TRUNCATE, 若文件长度超过最大长度NAME_LEN, 则返回
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif

	// 计算本目录中目录项数
	entries = (*dir)->i_size / (sizeof (struct dir_entry));
	*res_dir = NULL;

	// 检查文件名称长度，如果为0，直接返回。
	if (!namelen)
		return NULL;

	/* check for '..', as we might have to do some "magic" for it */
	// 检查名称为.., 表示父目录项
	if (namelen==2 && get_fs_byte(name)=='.' && get_fs_byte(name+1)=='.') {
		/* '..' in a pseudo-root results in a faked '.' (just change namelen) */
		// 如果当前进程的根基点指针就是指定目录，那么将文件名修改为.
		if ((*dir) == current->root)
			namelen=1;
		// 若目录i节点号等于ROOT_INO（1），说明是文件系统根节点，则取文件系统超级块 
		else if ((*dir)->i_num == ROOT_INO) {
			/* '..' over a mount-point results in 'dir' being exchanged for the mounted
  			 directory-inode. NOTE! We set mounted, so that we can iput the new dir */
			sb=get_super((*dir)->i_dev); // 获取文件系统超级块
			// 如果被安装到的i节点存在
			if (sb->s_imount) {
				iput(*dir); // 释放dir
				(*dir)=sb->s_imount; // dir执行被安装到的i节点
				(*dir)->i_count++;	// 该i节点的引用数+1
			}
		}
	}

	// 如果该i节点的第一个直接盘块号的i节点为0， 退出
	if (!(block = (*dir)->i_zone[0]))
		return NULL;
	// 读取该i节点的第一个块内容道高速缓存区。读取指定目录项数据块
	if (!(bh = bread((*dir)->i_dev,block)))
		return NULL;

	// 在目录项数据块中搜索指定的目录
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		// 如果当前目录项已经搜索完成，但是还没有搜索到
		if ((char *)de >= BLOCK_SIZE+bh->b_data) {
			// 释放缓冲区头
			brelse(bh);
			bh = NULL;

			// 读入下一个目录项数据块。 如果block为空，搜索下一个目录块。 
			if (!(block = bmap(*dir,i/DIR_ENTRIES_PER_BLOCK)) ||
			    !(bh = bread((*dir)->i_dev,block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			// 不为空，de指向该目录块
			de = (struct dir_entry *) bh->b_data;
		}

		// 文件名称匹配
		if (match(namelen,name,de)) {
			*res_dir = de;	// 将当前目录项返回
			return bh;	// 找到了
		}
		// 搜索下一个
		de++;	
		i++;
	}
	brelse(bh); // 释放高速缓冲区
	return NULL;
}

/*
 *	add_entry()
 *
 * adds a file entry to the specified directory, using the same
 * semantics as find_entry(). It returns NULL if it failed.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 *
 * 在指定目录项中增加一个文件目录项
 * @param dir 当前目录项
 * @param name 文件名称
 * @param namelen 名称长度
 * @param res_dir 目录项结构指针
 */
static struct buffer_head * add_entry(struct m_inode * dir,
	const char * name, int namelen, struct dir_entry ** res_dir)
{
	int block,i;
	struct buffer_head * bh;
	struct dir_entry * de;

	*res_dir = NULL;

	// 名称截断的宏配置 
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif

	// 名称长度为0， 不合法
	if (!namelen)
		return NULL;

	// 目录项第一个直接缓存块为0，返回。
	if (!(block = dir->i_zone[0]))
		return NULL;
	// 读取第一个直接缓存块
	if (!(bh = bread(dir->i_dev,block)))
		return NULL;

	// 遍历目录中的子目录
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (1) {
		// 如果当前目录项已经搜索完成，但是还没有搜索到
		if ((char *)de >= BLOCK_SIZE+bh->b_data) {
			brelse(bh); // 释放当前高速缓冲区
			bh = NULL;
			// 创建一个block。
			block = create_block(dir,i/DIR_ENTRIES_PER_BLOCK);
			// 创建失败
			if (!block)
				return NULL;
			// 读取这个块到高速缓冲区中，返回为NULL，跳过该块继续执行
			if (!(bh = bread(dir->i_dev,block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			// de指向高速缓冲区的数据地址
			de = (struct dir_entry *) bh->b_data;
		}

		// 如果当前的目录项序号 * 目录项结构大小超过了i节点的大小。 说明该目录项还没有使用
		if (i*sizeof(struct dir_entry) >= dir->i_size) {
			de->inode=0;	// 目录项的i节点为空
			dir->i_size = (i+1)*sizeof(struct dir_entry); // 目录项大小修改
			dir->i_dirt = 1;	// 已修改标记
			dir->i_ctime = CURRENT_TIME;	// 创建时间
		}
		
		// 如果该目录节点的i节点为空。 表示找到一个还没有使用的目录项
		if (!de->inode) {
			// 修改时间
			dir->i_mtime = CURRENT_TIME;
			// 复制文件名称。
			for (i=0; i < NAME_LEN ; i++)
				de->name[i]=(i<namelen)?get_fs_byte(name+i):0;
			// 已修改标志
			bh->b_dirt = 1;
			*res_dir = de;
			return bh;
		}
		de++;
		i++;
	}
	brelse(bh);
	return NULL;
}

/*
 *	get_dir()
 *
 * Getdir traverses the pathname until it hits the topmost directory.
 * It returns NULL on failure.
 * 
 * 根据指定的路径名取搜索，直到找到最顶端的目录
 * @param pathname 路径名
 * @return 目录的i节点指针
 */
static struct m_inode * get_dir(const char * pathname)
{
	char c;
	const char * thisname;
	struct m_inode * inode;
	struct buffer_head * bh;
	int namelen,inr,idev;
	struct dir_entry * de;

	// 如果没有根节点，或者跟节点的i_count为0（根节点的引用为0）
	if (!current->root || !current->root->i_count)
		panic("No root inode");
	// 如果当前进程的目录为空 或者当前进程的目录项引用计数为0
	if (!current->pwd || !current->pwd->i_count)
		panic("No cwd inode");

	// 从pathname中获取第一个字符，如果是以/开头的，表示绝对路径
	if ((c=get_fs_byte(pathname))=='/') {
		inode = current->root;
		pathname++;
	} else if (c) // 否则表示相对路径
		inode = current->pwd;
	else
		return NULL;	/* empty name is bad */

	// 增加i节点的引用计数
	inode->i_count++;
	while (1) {
		thisname = pathname;
		// 如果该i节点不是目录节点， 或者无法读取
		if (!S_ISDIR(inode->i_mode) || !permission(inode,MAY_EXEC)) {
			iput(inode);	// 释放i节点
			return NULL;
		}

		// namelen 为到下一个/的字符长度
		for(namelen=0;(c=get_fs_byte(pathname++))&&(c!='/');namelen++)
			/* nothing */ ;

		// 如果当前字符为NULL， 标志着搜索结束
		if (!c)
			return inode;

		// 查找给定文件名称的目录项。 如果高速缓存为空
		if (!(bh = find_entry(&inode,thisname,namelen,&de))) {
			iput(inode); // 释放i节点
			return NULL;
		}

		// 找到的目录项的i节点
		inr = de->inode;
		// 找到的目录项的设备号
		idev = inode->i_dev;

		// 释放高速缓存区
		brelse(bh);
		// 释放i节点
		iput(inode);
		if (!(inode = iget(idev,inr)))
			return NULL;
	}
}

/*
 *	dir_namei()
 *
 * dir_namei() returns the inode of the directory of the
 * specified name, and the name within that directory.
 * 
 * 返回指定目录名的i节点。已经在最顶级目录中的名称
 * @param pathname 路径名
 * @param namelen 名称长度，返回值
 * @param name 在顶级目录中的名称
 * @return dir 所在的目录
 */
static struct m_inode * dir_namei(const char * pathname,
	int * namelen, const char ** name)
{
	char c;
	const char * basename;
	struct m_inode * dir;

	// 取指定路径名最顶层目录的i节点，若出错则返回NULL
	if (!(dir = get_dir(pathname)))
		return NULL;

	// basename 是路径
	basename = pathname;
	while (c=get_fs_byte(pathname++))
		if (c=='/')
			basename=pathname;

	// 名称长度就是
	*namelen = pathname-basename-1;
	*name = basename;
	return dir;
}

/*
 *	namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 * 获取指定路径的i节点
 * @param pathname 路径名称
 * @return 对应的i节点
 */
struct m_inode * namei(const char * pathname)
{
	const char * basename;
	int inr,dev,namelen;
	struct m_inode * dir;
	struct buffer_head * bh;
	struct dir_entry * de;

	// 获取对应的目录节点
	if (!(dir = dir_namei(pathname,&namelen,&basename)))
		return NULL;
	// 长度为0，直接返回dir
	if (!namelen)			/* special case: '/usr/' etc */
		return dir;
	
	// 在指定目录中查找pathname。 
	bh = find_entry(&dir,basename,namelen,&de);
	// 缓冲区头为空
	if (!bh) {
		iput(dir); // 释放dir
		return NULL;
	}
	
	// 对应的i节点
	inr = de->inode;
	// 对应的设备号
	dev = dir->i_dev;
	brelse(bh);
	iput(dir);	// 释放dir

	// 获取对应的i节点 
	dir=iget(dev,inr);

	// 如果i节点存在，则修改时间，并置已修改标志
	if (dir) {
		dir->i_atime=CURRENT_TIME;
		dir->i_dirt=1;
	}
	return dir;
}

/*
 *	open_namei()
 *
 * namei for open - this is in fact almost the whole open-routine.
 * open 所使用的namei函数。
 * @param pathname 文件路径名
 * @param flag 文件打开标志
 * @param mode 文件访问许可标志
 * @param res_inode 文件路径名的i节点指针
 */
int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode)
{
	const char * basename;
	int inr,dev,namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// 文件模式是只读，文件截0标志（O_TRUNC）， 修改为只写标志
	if ((flag & O_TRUNC) && !(flag & O_ACCMODE))
		flag |= O_WRONLY;
	// 使用进程的文件访问许可屏蔽码，屏蔽掉给定模式中的相应位，并添上普通文件标志
	mode &= 0777 & ~current->umask;
	mode |= I_REGULAR;

	// 根据路径名得到对应的目录项 
	if (!(dir = dir_namei(pathname,&namelen,&basename)))
		return -ENOENT;

	// 名称长度为0 
	if (!namelen) {			/* special case: '/usr/' etc */
		// 如果打开模式不是创建，截0， 则表示打开一个目录名，直接返回该目录的i节点。并退出
		if (!(flag & (O_ACCMODE|O_CREAT|O_TRUNC))) {
			*res_inode=dir;
			return 0;
		}
		// 释放i节点，退出
		iput(dir);
		return -EISDIR;
	}

	// 在dir对应的目录中取文件目录，目录项的高速缓冲区
	bh = find_entry(&dir,basename,namelen,&de);
	// 高速缓冲区为空，表示没有找到对应文件名的目录项，因此只能是创建文件操作
	if (!bh) {
		// 非创建标志
		if (!(flag & O_CREAT)) {
			iput(dir);
			return -ENOENT;
		}

		// 权限标志
		if (!permission(dir,MAY_WRITE)) {
			iput(dir);
			return -EACCES;
		}
		// 申请新的inode
		inode = new_inode(dir->i_dev);
		if (!inode) {
			iput(dir);
			return -ENOSPC;
		}

		// 设置inode的uid，mode，置已修改标志
		inode->i_uid = current->euid;
		inode->i_mode = mode;
		inode->i_dirt = 1;
		// 在指定目录dir中添加 新目录项
		bh = add_entry(dir,basename,namelen,&de);
		// 添加不成功
		if (!bh) {
			inode->i_nlinks--; // 应用链接计数减1
			iput(inode);
			iput(dir);
			return -ENOSPC;
		}

		// i节点号为新申请到的i节点的号码
		de->inode = inode->i_num;
		// 修改标记
		bh->b_dirt = 1;
		// 
		brelse(bh);
		iput(dir);
		*res_inode = inode;
		return 0;
	}
	inr = de->inode;	// i节点号
	dev = dir->i_dev;	// 设备号
	brelse(bh);
	iput(dir);

	// 独占 使用标志，文件已存在错误
	if (flag & O_EXCL)
		return -EEXIST;

	// 读取i节点
	if (!(inode=iget(dev,inr)))
		return -EACCES;

	// 如果i节点是目录，访问模式是只写活读写，或者没有访问的许可权限，则释放该i节点。
	if ((S_ISDIR(inode->i_mode) && (flag & O_ACCMODE)) ||
	    !permission(inode,ACC_MODE(flag))) {
		iput(inode);
		return -EPERM;
	}

	// 访问时间更新
	inode->i_atime = CURRENT_TIME;
	// 截断
	if (flag & O_TRUNC)
		truncate(inode); // fs/trancate.c中
	*res_inode = inode;
	return 0;
}

/**
 * 系统调用，创建一个特殊文件活普通文件节点
 * @param filename 名称
 * @param mode 访问许可，创建节点的类型
 * @param dev 设备号
 * @return 0:成功， 否则返回出错号
 */
int sys_mknod(const char * filename, int mode, int dev)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	
	// 如果不是超级用户，返回出错号
	if (!suser())
		return -EPERM;

	// 如果找不到对应路径名目录的i节点， 返回出错号
	if (!(dir = dir_namei(filename,&namelen,&basename)))
		return -ENOENT;

	// 如果最顶端的文件名长度为0， 说明给出的路径名最后没有指定文件名
	if (!namelen) {
		iput(dir); // 释放该目录i节点，
		return -ENOENT; // 返回错误号
	}

	// 如果没有在该目录的写权限，释放该目录i节点，返回错误号
	if (!permission(dir,MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}

	// 如果对应路径名上最后的文件名目录项已经存在。 
	bh = find_entry(&dir,basename,namelen,&de);
	if (bh) {
		brelse(bh);	// 释放高速缓存区
		iput(dir);	// 释放i节点
		return -EEXIST;	// 放回文件已经存在
	}

	// 新建i节点，如果不成功，则释放目录项，放回无空间错误
	inode = new_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}

	// 设置新节点的属性模式。 如果要创建的是块设备文件或字符设备文件。令i节点的直接块0 等于设备号
	inode->i_mode = mode;
	if (S_ISBLK(mode) || S_ISCHR(mode))
		inode->i_zone[0] = dev;

	// 访问时间和修改时间更新
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	// 修改标志更新
	inode->i_dirt = 1;

	// 添加文件目录项
	bh = add_entry(dir,basename,namelen,&de);
	// 添加失败
	if (!bh) {
		iput(dir);			// 释放该目录
		inode->i_nlinks=0;	// inode 没有在使用
		iput(inode);		// 释放inode
		return -ENOSPC;		// 返回出错号
	}

	// 目录项指向该i节点的
	de->inode = inode->i_num;
	// 修改高度缓存的已更新标志
	bh->b_dirt = 1;
	// 释放目录项，inode，高速缓冲区
	iput(dir);
	iput(inode);
	brelse(bh);
	return 0;
}

/**
 * 系统调用，建立一个目录
 * @param pathname 路径名称
 * @param mode 访问权限
 */
int sys_mkdir(const char * pathname, int mode)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh, *dir_block;
	struct dir_entry * de;

	// 如果不是超级用户，返回错误号
	if (!suser())
		return -EPERM;
	
	// 获取指定路径父目录的i节点
	if (!(dir = dir_namei(pathname,&namelen,&basename)))
		return -ENOENT;

	// 如果最顶端的文件名长度为0， 说明给出的路径名最后没有指定文件名
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}

	// 如果没有dir的写访问权限
	if (!permission(dir,MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}

	// 查找对应的目录项，如果已经存在，返回错误号
	bh = find_entry(&dir,basename,namelen,&de);
	if (bh) {
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}

	// 新建inode。
	inode = new_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}

	// 目录项的i_size（文件长度）为32， 一个目录项的大小
	inode->i_size = 32;
	// 已修改标志 置1
	inode->i_dirt = 1;
	// 修改时间和访问时间更新
	inode->i_mtime = inode->i_atime = CURRENT_TIME;

	// 为新生成的i节点申请一个磁盘块。 令第一个直接块指针等于这个申请的磁盘块。
	// 申请失败，就释放父目录的i节点，释放新节点
	if (!(inode->i_zone[0]=new_block(inode->i_dev))) {
		iput(dir);
		inode->i_nlinks--;	// 新节点没有在使用
		iput(inode);
		return -ENOSPC;
	}
	inode->i_dirt = 1;	// 新节点已修改标志
	// 读取新生清的磁盘块。
	if (!(dir_block=bread(inode->i_dev,inode->i_zone[0]))) {
		iput(dir);									// 释放负目录
		free_block(inode->i_dev,inode->i_zone[0]); // 释放磁盘块
		inode->i_nlinks--;							// inode没有在使用中
		iput(inode);								// 释放inode
		return -ERROR;
	}

	// 强制转换目录项
	de = (struct dir_entry *) dir_block->b_data;
	de->inode=inode->i_num; // 指向当前inode节点号
	strcpy(de->name,".");

	// 下一个目录项
	de++;
	de->inode = dir->i_num;
	strcpy(de->name,"..");

	inode->i_nlinks = 2;	// 被两个地方使用
	dir_block->b_dirt = 1;	// 已修改标志

	brelse(dir_block);		// 释放block
	// i节点的模式修改。 文件
	inode->i_mode = I_DIRECTORY | (mode & 0777 & ~current->umask);
	inode->i_dirt = 1;	// i节点的已修改标志

	// 在父目录中添加文件目录项
	bh = add_entry(dir,basename,namelen,&de);
	if (!bh) {	// 添加失败的处理， 就是释放已经获取到的资源。
		iput(dir);
		free_block(inode->i_dev,inode->i_zone[0]);
		inode->i_nlinks=0;
		iput(inode);
		return -ENOSPC;
	}

	// 文件目录项的i节点指向i节点号
	de->inode = inode->i_num;
	// 高速缓存已修改标志
	bh->b_dirt = 1;
	// 父目录引用次数
	dir->i_nlinks++;
	// 父目录已修改标记
	dir->i_dirt = 1;
	iput(dir);	// 释放父目录
	iput(inode);	// 释放i节点
	brelse(bh);		// 释放高速缓存区
	return 0;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 * 检查指定的i节点是是否为空目录
 */
static int empty_dir(struct m_inode * inode)
{
	int nr,block;
	int len;
	struct buffer_head * bh;
	struct dir_entry * de;

	// 文件数量=文件长度/文件目录项结构大小
	len = inode->i_size / sizeof (struct dir_entry);
	// 长度小于2，或者没有直接块，或者读取第一个直接块失败
	if (len<2 || !inode->i_zone[0] ||
	    !(bh=bread(inode->i_dev,inode->i_zone[0]))) {
	    	printk("warning - bad directory on dev %04x\n",inode->i_dev);
		return 0;
	}

	// 获取子目录的第一个项
	de = (struct dir_entry *) bh->b_data;
	// 检查i节点号，.和..目录
	if (de[0].inode != inode->i_num || !de[1].inode || 
	    strcmp(".",de[0].name) || strcmp("..",de[1].name)) {
	    	printk("warning - bad directory on dev %04x\n",inode->i_dev);
		return 0;
	}
	
	// 跳过当前目录和父目录，.和..
	nr = 2;
	de += 2;

	// 遍历子目录
	while (nr<len) {
		// 如果超过了块大小，意味着要访问下一个块了
		if ((void *) de >= (void *) (bh->b_data+BLOCK_SIZE)) {
			brelse(bh);

			// 获取下一个缓冲区的块。
			block=bmap(inode,nr/DIR_ENTRIES_PER_BLOCK);

			// 如果相应块没有被使用，比如：文件已经被删除。 则跳过
			if (!block) {
				nr += DIR_ENTRIES_PER_BLOCK;
				continue;
			}

			// 读取下一个缓冲区
			if (!(bh=bread(inode->i_dev,block)))
				return 0;
			de = (struct dir_entry *) bh->b_data;
		}

		// 如果该目录的i节点字段不等于0，表示该目录项正在使用中，释放该高速缓冲区，返回0
		if (de->inode) {
			brelse(bh);
			return 0;
		}
		// 指向下一个字目录项
		de++;
		nr++;
	}
	// 到这里说明该目录中没有找到已用的目录项，释放缓冲区，放回1
	brelse(bh);
	return 1;
}

/**
 * 系统调用，删除一个目录
 */
int sys_rmdir(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// 如果不是超级用户
	if (!suser())
		return -EPERM;

	// 获取对应的父目录的i节点
	if (!(dir = dir_namei(name,&namelen,&basename)))
		return -ENOENT;

	// 顶端文件名长度为0，表示没有指定文件名
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}

	// 检查是否有写权限
	if (!permission(dir,MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}

	// 找到对应的文件目录项。 路径名上最后的文件名对应的目录项不存在
	bh = find_entry(&dir,basename,namelen,&de);
	// 如果找到
	if (!bh) {
		iput(dir);
		return -ENOENT;
	}

	// 获取目录项的inode信息
	if (!(inode = iget(dir->i_dev, de->inode))) {
		iput(dir);
		brelse(bh);
		return -EPERM;
	}

	// 如果设置了受限删除， 并且进程有效用户id 不等于该i节点用户id，表示没有权限删除该目录
	if ((dir->i_mode & S_ISVTX) && current->euid &&
	    inode->i_uid != current->euid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}

	// 如果被删除的目录项的i节点的设备号不等于包含该父目录的设备号。 并且i节点的引用计数大于1（表示有符号链接等）
	// 不能删除该目录
	if (inode->i_dev != dir->i_dev || inode->i_count>1) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}

	// i节点和父目录的i节点相等。 表示试图删除.的目录项。 
	if (inode == dir) {	/* we may not delete ".", but "../dir" is ok */
		iput(inode);
		iput(dir);
		brelse(bh);
		return -EPERM;
	}

	// 如果不是目录
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -ENOTDIR;
	}

	// 该目录不为空
	if (!empty_dir(inode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -ENOTEMPTY;
	}

	// 要删除的目录i节点链接数不等于2，显示警告信息
	if (inode->i_nlinks != 2)
		printk("empty directory has nlink!=2 (%d)",inode->i_nlinks);

	de->inode = 0;	// 移除该目录项的inode。
	bh->b_dirt = 1;	// 已更新标志
	brelse(bh);		// 释放高速缓冲区
	inode->i_nlinks=0;	// 链接数为0
	inode->i_dirt=1;	// 已修改标志
	dir->i_nlinks--;	// 父目录的链接数减1
	dir->i_ctime = dir->i_mtime = CURRENT_TIME;	// 父目录的改变时间和修改时间
	dir->i_dirt=1;		// 父目录的已更新标志
	iput(dir);
	iput(inode);
	return 0;
}

/**
 * 系统调用，删除文件名，以及可能的相关文件
 * 从文件系统删除一个名字。如果是一个文件的最后一个链接，并且没有进程正载打开这个文件。则该文件也要删除
 * @param name 要删除的文件
 * @return 0:成功，否则返回错误号
 */
int sys_unlink(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// 找不到不节点
	if (!(dir = dir_namei(name,&namelen,&basename)))
		return -ENOENT;

	// 如果最顶端的文件名长度为0， 说明给出的路径名最后没有指定文件名，释放该目录i节点
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}

	// 检查该i节点的写权限
	if (!permission(dir,MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}

	// 查找该文件对应的文件目录项 
	bh = find_entry(&dir,basename,namelen,&de);
	// 如果为空，表示对应的文件不存在
	if (!bh) {
		iput(dir);
		return -ENOENT;
	}

	// 找到对应的i节点，如果i节点为空。
	if (!(inode = iget(dir->i_dev, de->inode))) {
		iput(dir);
		brelse(bh);
		return -ENOENT;
	}

	// 文件设置了受限删除权限，并且不是超级用户，并且进程有效用户id不是inode的用户id，并且进程的有效用户id不是父目录的用户id
	if ((dir->i_mode & S_ISVTX) && !suser() &&
	    current->euid != inode->i_uid &&
	    current->euid != dir->i_uid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}

	// 被删除的是一个文件
	if (S_ISDIR(inode->i_mode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -EPERM;
	}

	// 如果inode的链接数为0，表示该文件不存在。将该文件的链接数置为1
	if (!inode->i_nlinks) {
		printk("Deleting nonexistent file (%04x:%d), %d\n",
			inode->i_dev,inode->i_num,inode->i_nlinks);
		inode->i_nlinks=1;
	}
	de->inode = 0;		// 文件目录的inode为0，释放文件目录项
	bh->b_dirt = 1;		// 缓冲区已修改标记
	brelse(bh);			// 释放高速缓冲区
	inode->i_nlinks--;	// inode的链接数减去1
	inode->i_dirt = 1;	// inode 的已修改标记
	inode->i_ctime = CURRENT_TIME;	// 修改时间设置为当前时间
	iput(inode);
	iput(dir);
	return 0;
}

/**
 * 系统调用， 为文件建立一个文件名。
 * 为已经存在的文件建立一个新链接，也成为hard link
 * @param oldname 已经存在的文件名
 * @param newname 新的文件名
 * @return 0:成功，否则返回错误码
 */
int sys_link(const char * oldname, const char * newname)
{
	struct dir_entry * de;
	struct m_inode * oldinode, * dir;
	struct buffer_head * bh;
	const char * basename;
	int namelen;

	// 旧文件的i节点
	oldinode=namei(oldname);
	if (!oldinode)
		return -ENOENT;

	// 如果是目录，则返回错误码
	if (S_ISDIR(oldinode->i_mode)) {
		iput(oldinode);
		return -EPERM;
	}

	// 获取新节点的目录对应的i节点
	dir = dir_namei(newname,&namelen,&basename);
	// 新节点目录的i节点不存在
	if (!dir) {
		iput(oldinode);
		return -EACCES;
	}
	
	// 如果最顶端的文件名长度为0， 说明给出的路径名最后没有指定文件名，释放该目录i节点
	if (!namelen) {
		iput(oldinode);
		iput(dir);
		return -EPERM;
	}

	// 如果新旧节点不在同一个设备中
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}

	// 检查新节点父目录的写权限
	if (!permission(dir,MAY_WRITE)) {
		iput(dir);
		iput(oldinode);
		return -EACCES;
	}

	// 在父目录中查找对应的写权限
	bh = find_entry(&dir,basename,namelen,&de);
	// 目录的缓冲区头为NULL，表示父目录不存在（访问出错）
	if (bh) {
		brelse(bh);
		iput(dir);
		iput(oldinode);
		return -EEXIST;
	}

	// 添加新的目录项，新文件
	bh = add_entry(dir,basename,namelen,&de);
	// 添加不成功
	if (!bh) {
		iput(dir);
		iput(oldinode);
		return -ENOSPC;
	}

	// 添加成功
	de->inode = oldinode->i_num;	// 新的文件使用旧文件的节点号
	bh->b_dirt = 1;					// 缓冲区头已更新标志
	brelse(bh);						// 释放缓冲区
	iput(dir);						// 释放父目录
	oldinode->i_nlinks++;			// 源文件链接数+1
	oldinode->i_ctime = CURRENT_TIME;	// 源文件修改文件更新为当前时间
	oldinode->i_dirt = 1;				// 源文件的已修改标志
	iput(oldinode);						// 释放源文件，会回写设备
	return 0;
}
