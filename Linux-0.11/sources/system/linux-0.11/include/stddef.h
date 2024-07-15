#ifndef _STDDEF_H
#define _STDDEF_H

// 两个指针相减的结果类型
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;
#endif

// sizeof的返回类型
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

// NULL的定义
#undef NULL
#define NULL ((void *)0)

// 成员在类型中的偏移位置
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif
