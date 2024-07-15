#ifndef _HEAD_H
#define _HEAD_H

/**
 * 段描述符的数据结构。该结构仅说明每个描述符是有8恶搞字节构成，每个表描述符表共有256项
 */
typedef struct desc_struct {
	unsigned long a,b;
} desc_table[256];

// 内存页目录数组， 每个目录项4字节，从物理地址0开始
extern unsigned long pg_dir[1024];
// 中断描述符表，全局描述符表
extern desc_table idt,gdt;

// 全局描述符表第0项，不用
#define GDT_NUL 0
// 第一项，内核代码段描述符
#define GDT_CODE 1
// 第二项，内核数据段描述符项
#define GDT_DATA 2
// 第三项，系统段描述符， Linux中没有使用
#define GDT_TMP 3

// 每个局部描述符表的第0项，不用
#define LDT_NUL 0
// 第一项，用户程序代码段描述符项
#define LDT_CODE 1
// 第二项，用户程序数据段描述符项
#define LDT_DATA 2

#endif
