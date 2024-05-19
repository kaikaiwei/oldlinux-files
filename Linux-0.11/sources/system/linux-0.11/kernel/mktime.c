/*
 *  linux/kernel/mktime.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <time.h>

/*
 * This isn't the library routine, it is only used in the kernel.
 * as such, we don't care about years<1970 etc, but assume everything
 * is ok. Similarly, TZ etc is happily ignored. We just do everything
 * as easily as possible. Let's find something public for the library
 * routines (although I think minix times is public).
 */
/*
 * PS. I hate whoever though up the year 1970 - couldn't they have gotten
 * a leap-year instead? I also hate Gregorius, pope or no. I'm grumpy.
 */
#define MINUTE 60				// 1分钟的秒数
#define HOUR (60*MINUTE)		// 1小时的秒数
#define DAY (24*HOUR)			// 1填的秒数
#define YEAR (365*DAY)			// 1年的秒数

// 每个月开始时的秒数
/* interestingly, we assume leap-years */
static int month[12] = {
	0,
	DAY*(31),
	DAY*(31+29),
	DAY*(31+29+31),
	DAY*(31+29+31+30),
	DAY*(31+29+31+30+31),
	DAY*(31+29+31+30+31+30),
	DAY*(31+29+31+30+31+30+31),
	DAY*(31+29+31+30+31+30+31+31),
	DAY*(31+29+31+30+31+30+31+31+30),
	DAY*(31+29+31+30+31+30+31+31+30+31),
	DAY*(31+29+31+30+31+30+31+31+30+31+30)
};

// 计算从1970年1月1日时起到开机当日经过的秒数，作为开机时间。
long kernel_mktime(struct tm * tm)
{
	long res;
	int year;

	year = tm->tm_year - 70; // 1970-1-1，按照这个初始时间来算的。
/* magic offsets (y+1) needed to get leapyears right.*/
	res = YEAR*year + DAY*((year+1)/4); // 这些年经过的秒数
	res += month[tm->tm_mon];	// 得到这个月经过的秒数

	// 瑞月的话，要减去一天
/* and (y+2) here. If it wasn't a leap-year, we have to adjust */
	if (tm->tm_mon>1 && ((year+2)%4))
		res -= DAY;
	// 增加天，小时，分钟，秒级别的时间。
	res += DAY*(tm->tm_mday-1);
	res += HOUR*tm->tm_hour;
	res += MINUTE*tm->tm_min;
	res += tm->tm_sec;
	return res;
}
