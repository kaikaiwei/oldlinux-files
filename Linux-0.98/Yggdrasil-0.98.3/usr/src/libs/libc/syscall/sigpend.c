#define __LIBRARY__
#include <unistd.h>

_syscall1(int,sigpending,sigset_t *,set)
