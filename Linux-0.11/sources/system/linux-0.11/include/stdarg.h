#ifndef _STDARG_H
#define _STDARG_H

// 重定义 va_list
typedef char *va_list;

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */
// 给出类型为type的arg参数列表所要求空间容量
// 取整后的容量，4的整数倍
#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

// 使得ap指向传递给函数的可变参数表的第一个参数
#ifndef __sparc__
#define va_start(AP, LASTARG) 						\
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#else
#define va_start(AP, LASTARG) 						\
 (__builtin_saveregs (),						\
  AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#endif

// 用于被调用函数完成一次正常返回。 va_end可以修改ap，使其在重新调用va_start之前不能被使用
// 必须在va_arg读完所有的参数后在被调用。 gnulib中定义
void va_end (va_list);		/* Defined in gnulib */
#define va_end(AP)

// 用于扩展表达式，使其与下一个被传递参数具有相同的类型和值。
// 对于缺省值，va_arg可以用字符/无符号字符和浮点类型。
// 第一次使用时，它返回表中的第一个参数，后续的每次调用都将返回表中的下一个参数。
#define va_arg(AP, TYPE)						\
 (AP += __va_rounded_size (TYPE),					\
  *((TYPE *) (AP - __va_rounded_size (TYPE))))

#endif /* _STDARG_H */
