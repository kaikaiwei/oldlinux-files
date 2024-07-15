#ifndef _TERMIOS_H
#define _TERMIOS_H

#define TTY_BUF_SIZE 1024

// posix1 定义的所有11个输入标志
/* 0x54 is just a magic number to make these relatively uniqe ('T') */

#define TCGETS		0x5401
#define TCSETS		0x5402
#define TCSETSW		0x5403
#define TCSETSF		0x5404
#define TCGETA		0x5405
#define TCSETA		0x5406
#define TCSETAW		0x5407
#define TCSETAF		0x5408
#define TCSBRK		0x5409
#define TCXONC		0x540A
#define TCFLSH		0x540B
#define TIOCEXCL	0x540C
#define TIOCNXCL	0x540D
#define TIOCSCTTY	0x540E
#define TIOCGPGRP	0x540F
#define TIOCSPGRP	0x5410
#define TIOCOUTQ	0x5411
#define TIOCSTI		0x5412
#define TIOCGWINSZ	0x5413
#define TIOCSWINSZ	0x5414
#define TIOCMGET	0x5415
#define TIOCMBIS	0x5416
#define TIOCMBIC	0x5417
#define TIOCMSET	0x5418
#define TIOCGSOFTCAR	0x5419
#define TIOCSSOFTCAR	0x541A
#define TIOCINQ		0x541B

/**
 * 窗体大小
 */
struct winsize {
	unsigned short ws_row;		// 行数
	unsigned short ws_col;		// 列数
	unsigned short ws_xpixel;	// 窗体宽的像素值
	unsigned short ws_ypixel;	// 窗体高的像素值
};

// 控制字符数组长度
#define NCC 8

/**
 * AT&T系统V中定义的。 使用短整型定义标志集
 */
struct termio {
	unsigned short c_iflag;		/* input mode flags  输入模式标志*/
	unsigned short c_oflag;		/* output mode flags  输出模式标志*/
	unsigned short c_cflag;		/* control mode flags  控制模式标志*/
	unsigned short c_lflag;		/* local mode flags  本地模式标志*/
	unsigned char c_line;		/* line discipline  线路规程（速率）*/
	unsigned char c_cc[NCC];	/* control characters 控制字符数组 */
};

#define NCCS 17
/**
 * POSIX 标准定义，使用长整型数定义模式标志集
 */
struct termios {
	unsigned long c_iflag;		/* input mode flags */
	unsigned long c_oflag;		/* output mode flags */
	unsigned long c_cflag;		/* control mode flags */
	unsigned long c_lflag;		/* local mode flags */
	unsigned char c_line;		/* line discipline */
	unsigned char c_cc[NCCS];	/* control characters */
};

// c_cc数组中的字符
/* c_cc characters */
// \003 中断字符
#define VINTR 0
// \034 退出字符
#define VQUIT 1
// \177 擦除字符
#define VERASE 2
// \025 终止字符
#define VKILL 3
// \004 文件结束
#define VEOF 4
// \0	定时器值
#define VTIME 5
// \0	定时器值
#define VMIN 6
// \0	交换字符
#define VSWTC 7
// \021 开始字符
#define VSTART 8
// \023	停止字符
#define VSTOP 9
// \032 挂起字符
#define VSUSP 10
// \0 行结束字符
#define VEOL 11
// \022 重显示字符
#define VREPRINT 12
// \017	丢弃字符
#define VDISCARD 13
// \027 单次擦除字符
#define VWERASE 14
// \026	下一行字符
#define VLNEXT 15
// \0	行结束字符
#define VEOL2 16

/* c_iflag bits */
// 输入模式的各种标记常量
// 输入时忽略break条件
#define IGNBRK	0000001
// 在break时产生sigint信号
#define BRKINT	0000002
// 忽略奇偶校验出错的字符
#define IGNPAR	0000004
// 标记奇偶校验错
#define PARMRK	0000010
// 允许输入奇偶校验
#define INPCK	0000020
// 屏蔽字符第8位
#define ISTRIP	0000040
// 输入时换行符NL映射成回车符CR
#define INLCR	0000100
// 忽略回车符CR
#define IGNCR	0000200
// 在输入时将回车符CR映射成换行符NL
#define ICRNL	0000400
// 在输入时将大写字符转换成小写字符
#define IUCLC	0001000
// 允许开始/停止输入输出控制， XON/XOFF
#define IXON	0002000
// 输入队列满时响铃
#define IXANY	0004000
#define IXOFF	0010000
#define IMAXBEL	0020000

/* c_oflag bits */
// termios结构中输出模式字段c_oflag各种标志的符号常量
// 执行输出处理
#define OPOST	0000001
// 在输出时将小写字符转换成大写字符
#define OLCUC	0000002
// 在输出时将换行符NL映射成回车符
#define ONLCR	0000004
// 在输出时将回车符CR映射成换行符NL
#define OCRNL	0000010
// 在0列不输出回车符CR
#define ONOCR	0000020
// 换行符NL执行回车符的功能
#define ONLRET	0000040
// 延迟时使用填充字符，而不是使用时间延迟
#define OFILL	0000100
// 填充字符是ASCII字符DEL，如果没有设置，会使用ASCII的NULL
#define OFDEL	0000200
// 选择换行延迟
#define NLDLY	0000400
// 换行延迟类型0
#define   NL0	0000000
#define   NL1	0000400
// 选择回车延时
#define CRDLY	0003000
// 回车延迟类型0
#define   CR0	0000000
#define   CR1	0001000
#define   CR2	0002000
#define   CR3	0003000
// 选择水平制表符延迟
#define TABDLY	0014000
// 选择水平制表符延迟类型
#define   TAB0	0000000
#define   TAB1	0004000
#define   TAB2	0010000
#define   TAB3	0014000
// 将制表符TAB换成空格，该值表示空格数
#define   XTABS	0014000
// 选择退格延迟
#define BSDLY	0020000
// 退格延迟类型
#define   BS0	0000000
#define   BS1	0020000
// 纵向制表延迟
#define VTDLY	0040000
// 纵向制表延迟类型
#define   VT0	0000000
#define   VT1	0040000
// 选择换页延迟
#define FFDLY	0040000
// 选择换页延迟类型
#define   FF0	0000000
#define   FF1	0040000

/* c_cflag bit meaning */
// 控制标志常量
// 传输速率位屏蔽码
#define CBAUD	0000017
// 挂断线路
#define  B0	0000000		/* hang up */
// 波特率50
#define  B50	0000001
// 波特率75
#define  B75	0000002
// 波特率110
#define  B110	0000003
// 波特率134
#define  B134	0000004
// 波特率150
#define  B150	0000005
// 波特率200
#define  B200	0000006
// 波特率300
#define  B300	0000007
// 波特率600
#define  B600	0000010
// 波特率1200
#define  B1200	0000011
// 波特率1800
#define  B1800	0000012
// 波特率2400
#define  B2400	0000013
// 波特率4800
#define  B4800	0000014
// 波特率9600
#define  B9600	0000015
// 波特率19200
#define  B19200	0000016
// 波特率38400
#define  B38400	0000017
// 扩展波特率A
#define EXTA B19200
// 扩展波特率B
#define EXTB B38400
// 字符位宽度屏蔽码
#define CSIZE	0000060
// 每个字符5比特位
#define   CS5	0000000
// 每个字符6比特位
#define   CS6	0000020
// 每个字符7比特位
#define   CS7	0000040
// 每个字符8比特位
#define   CS8	0000060
// 设置两个停止位
#define CSTOPB	0000100
// 允许接收
#define CREAD	0000200
// 开启输出时长生奇偶位， 输入时进行奇偶校验
#define CPARENB	0000400
// 输入/输入校验时奇校验
#define CPARODD	0001000
// 最后进程关闭后挂断
#define HUPCL	0002000
// 忽略调制解调器modem控制线路
#define CLOCAL	0004000
// 输入波特率
#define CIBAUD	03600000		/* input baud rate (not used) */
// 流控制
#define CRTSCTS	020000000000		/* flow control */


// 开启输入时产生奇偶位，输入时进行奇偶验证
#define PARENB CPARENB
// 输入/输入校验时奇校验
#define PARODD CPARODD

/* c_lflag bits */
// 本地模式标志位
// 当收到字符intr，quit， susp，dsusp时，产生相应的信号
#define ISIG	0000001
// 开启规范模式（熟模式）
#define ICANON	0000002
// 若设置了ICANON， 则终端是大写字符的
#define XCASE	0000004
// 回显输入字符
#define ECHO	0000010
// 若设置了ICANON， 则ERASE/WERASE将擦除前一字符/单词
#define ECHOE	0000020
// 若设置了ECANON， 则KILL字符将擦除当前行
#define ECHOK	0000040
// 若设置了ECANON， 则即使ECHO没有开启也回显NL字符
#define ECHONL	0000100
// 当生成sigint或sigquit信号时不刷新输入输出队列，当生成sigsusp信号时，刷新输入队列
#define NOFLSH	0000200
// 发送sigttou信号到后台进程的进程组， 该后台进程时图写自己的控制终端
#define TOSTOP	0000400
// 若设置了ECHO，则除TAB， NL，START， STOP意外以外的ascii控制信号将被显示^X  X是控制字符+0400
#define ECHOCTL	0001000
// 若设置了ICANON， IECHO， 则字符在擦除时，所有字符被回显
#define ECHOPRT	0002000
// 若设置了ICANON， 则通过KILL进行擦除的所有字符被回显
#define ECHOKE	0004000
// 输出被刷新。通过键入DISCARD字符，该标志被翻转
#define FLUSHO	0010000
// 当下一个字符时读时，输入队列中的所有字符将被重置
#define PENDIN	0040000
// 开启实现时定义的输入处理
#define IEXTEN	0100000

/* modem lines */
// modem线路信号符号常量
// 线路允许 Line Enable
#define TIOCM_LE	0x001
// 数据终端就绪 Data Terminal Ready
#define TIOCM_DTR	0x002
// 请求发送 Request to Send
#define TIOCM_RTS	0x004
// 串行数据发送 Serial Transfer
#define TIOCM_ST	0x008
// 串行数据接收 Serial Transfer
#define TIOCM_SR	0x010
// 清除发送 Clear To Send
#define TIOCM_CTS	0x020
// 载波检测Carrier Detect
#define TIOCM_CAR	0x040
// 响铃指示 Ring indicate
#define TIOCM_RNG	0x080
// 数据设备就绪 Data Set Ready
#define TIOCM_DSR	0x100
//
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* tcflow() and TCXONC use these */
// 挂起输出
#define	TCOOFF		0
// 重起被挂起的输出
#define	TCOON		1
// 系统传输一个STOP字符，使设备停止向系统传输数据
#define	TCIOFF		2
// 系统传输一个START字符，使设备开始向系统传输数据
#define	TCION		3

/* tcflush() and TCFLSH use these */
// 清接收到的数据但不读
#define	TCIFLUSH	0
// 清已写的数据但不传送
#define	TCOFLUSH	1
// 清接收到的数据但不读，清已写的数据但不传送
#define	TCIOFLUSH	2

/* tcsetattr uses these  tcsetattr函数使用这些符号常量 */
// 清接收到的数据但不读
#define	TCSANOW		0
// 清已写的数据但不传送
#define	TCSADRAIN	1
// 清接收到的数据但不读，清已写的数据但不传送
#define	TCSAFLUSH	2

// 波特率数值类型
typedef int speed_t;

// 返回termios_p所指的termios结构中的接收波特率
extern speed_t cfgetispeed(struct termios *termios_p);
// 返回termios_p所指的termios结构中的发送波特率
extern speed_t cfgetospeed(struct termios *termios_p);
// 设置termios的接收波特率
extern int cfsetispeed(struct termios *termios_p, speed_t speed);
// 设置termios的发送波特率
extern int cfsetospeed(struct termios *termios_p, speed_t speed);
// 等待fildes所指对象已写数据被传送出去
extern int tcdrain(int fildes);
// 挂起/重起 fields所指对象数据的接收和发送
extern int tcflow(int fildes, int action);
// 丢弃fildes指定对象所有已写但还没传送以及所有已收但还没有读取的数据
extern int tcflush(int fildes, int queue_selector);
// 获取句柄fildes对应对象的参数，并将其保存在termios_p所指的地方
extern int tcgetattr(int fildes, struct termios *termios_p);
// 如果终端使用一步串行数据传输，则在一定时间内连续传输一系列0值比特位
extern int tcsendbreak(int fildes, int duration);
// 使用termios指针结构，设置与终端相关的参数
extern int tcsetattr(int fildes, int optional_actions,
	struct termios *termios_p);

#endif
