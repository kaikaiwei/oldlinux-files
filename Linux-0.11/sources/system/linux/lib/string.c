/*
 *  linux/lib/string.c
 *
 *  (C) 1991  Linus Torvalds
 */

#ifndef __GNUC__
#error I want gcc!
#endif

#define extern
#define inline
#define __LIBRARY__
#include <string.h>

// 操作函数在.h中实现了，因此这里只包含了string.h
