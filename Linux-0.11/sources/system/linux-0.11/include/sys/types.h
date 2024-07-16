#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

// 对象的大小
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

// 时间，以秒计时
#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif

// 指针
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;
#endif

// 空指针
#ifndef NULL
#define NULL ((void *) 0)
#endif

// 进程id
typedef int pid_t;
// 用户号
typedef unsigned short uid_t;
// 用户组号
typedef unsigned char gid_t;
// 设备号
typedef unsigned short dev_t;
// 文件序列号
typedef unsigned short ino_t;
// 文件属性
typedef unsigned short mode_t;
// 
typedef unsigned short umode_t;
// 链接计数
typedef unsigned char nlink_t;
typedef int daddr_t;
// 文件长度（大小）
typedef long off_t;
// 无符号字符类型
typedef unsigned char u_char;
// 无符号短整形类型
typedef unsigned short ushort;

// div操作
typedef struct { int quot,rem; } div_t;
// 长div操作
typedef struct { long quot,rem; } ldiv_t;

struct ustat {
	daddr_t f_tfree;
	ino_t f_tinode;
	char f_fname[6];
	char f_fpack[6];
};

#endif
