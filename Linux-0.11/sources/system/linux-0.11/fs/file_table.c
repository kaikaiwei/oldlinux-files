/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/fs.h>

// 就定义了文件表， NR_FILE为64。 表示整个系统中能够同时打开的文件只有64个
struct file file_table[NR_FILE];
