#ifndef _TIME_H
#define _TIME_H

#ifndef _TIME_T
#define _TIME_T
// 从1970年1月1日开始以秒计数的时间
typedef long time_t;
#endif

#ifndef _SIZE_T
#define _SIZE_T
// size_t不应该在这里吧。
typedef unsigned int size_t;
#endif

// 系统时钟滴答频率， 100Hz
#define CLOCKS_PER_SEC 100

// 从进程开始系统经过的始终滴答数
typedef long clock_t;


struct tm {
	int tm_sec;		// 秒，0-59
	int tm_min;		// 分钟数 0-59
	int tm_hour;	// 小时  0-59
	int tm_mday;	// 一个月的天数 [0,31]
	int tm_mon;		// 一年中的月份 [0,11]
	int tm_year;	// 从1900年开始的年数
	int tm_wday;	// 一星期中的某天,[0,6], 0表示星期天
	int tm_yday;	// 一年中的某天，[0, 365]
	int tm_isdst;	// 夏令时标志
};
// 确定处理器使用时间。返回程序所用处理器时间的近似值
clock_t clock(void);
// 取时间，
time_t time(time_t * tp);
// 计算时间差，time2与time1秒数的差值
double difftime(time_t time2, time_t time1);
// 将tm结构表示的时间转换成日历时间
time_t mktime(struct tm * tp);

// 将时间转换成一个字符串。返回执行串指针
char * asctime(const struct tm * tp);
// 将日历时间转换成一个字符串形式
char * ctime(const time_t * tp);
// 将日历时间转换成tm结构表示的utc时间
struct tm * gmtime(const time_t *tp);
// 将日历时间妆换成指定失去的时间
struct tm *localtime(const time_t * tp);
// 将tm表示的时间利用格式字符串fmt转换成最大长度为smax的字符串，最终结果保存在s中
size_t strftime(char * s, size_t smax, const char * fmt, const struct tm * tp);
// 初始化时间转换信息，使用环境变量TZ， 对zname变量进行初始化。与在失去相关的时间转换函数中将自动调用该函数
void tzset(void);

#endif
