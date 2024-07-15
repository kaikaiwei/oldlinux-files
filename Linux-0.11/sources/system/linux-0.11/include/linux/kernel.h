/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
 // 验证给定的地址开始的内存是否超西安，若超限则追加内存 kernel/fork.c
void verify_area(void * addr,int count);
// 显示内核出错信息，进入死循环
volatile void panic(const char * str);
// 标准打印输出
int printf(const char * fmt, ...);
// 内核专用打印信息函数
int printk(const char * fmt, ...);
// 向tty写字符串
int tty_write(unsigned ch,char * buf,int count);
// 通过内核内存分配函数 malloc.c
void * malloc(unsigned int size);
// 释放指定对象占用内存 malloc.c
void free_s(void * obj, int size);

// 释放函数
#define free(x) free_s((x), 0)

/*
 * This is defined as a macro, but at some point this might become a
 * real subroutine that sets a flag if it returns true (to do
 * BSD-style accounting where the process is flagged if it uses root
 * privs).  The implication of this is that you should do normal
 * permissions checks first, and check suser() last.
 */
 // 检测是否为超级用户。 先用常规检测，因为会造成BSD方式的记账处理
#define suser() (current->euid == 0)

