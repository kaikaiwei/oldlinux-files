/*
 *  linux/lib/write.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

/**
 * 写文件系统调用函数
 * 对应int write(int fd, const char * buf, off_t count)
 * @param fd 文件描述符
 * @param buf 写缓冲区指针
 * @param count 写字节数
 * @return 成功时返回写入的字节数， 出错时返回-1， 并置出错号
 */
_syscall3(int,write,int,fd,const char *,buf,off_t,count)
