/*
 * This file contains some defines for the floppy disk controller.
 * Various sources. Mostly "IBM Microcomputers: A Programmers
 * Handbook", Sanches and Canton.
 * 软盘系统常用到的一些参数以及所使用的io端口。
 * 由于软盘驱动器的控制比较繁琐。 需要参考微型计算机控制接口远离的数据。了解FOC软盘控制器的公族欧远离。
 */
#ifndef _FDREG_H
#define _FDREG_H

// 软盘函数原型说明
extern int ticks_to_floppy_on(unsigned int nr);
extern void floppy_on(unsigned int nr);
extern void floppy_off(unsigned int nr);
extern void floppy_select(unsigned int nr);
extern void floppy_deselect(unsigned int nr);

/* Fd controller regs. S&C, about page 340 */
// 软盘控制器，寄存端口。 
#define FD_STATUS	0x3f4
#define FD_DATA		0x3f5
#define FD_DOR		0x3f2		/* Digital Output Register 数字输出寄存器 */
#define FD_DIR		0x3f7		/* Digital Input Register (read)  数字输入寄存器 */
#define FD_DCR		0x3f7		/* Diskette Control Register (write) 传输率控制寄存器 */

/* Bits of main status register */
// 主状态寄存器各比特位的含义
#define STATUS_BUSYMASK	0x0F		/* drive busy mask 驱动器忙，每位对应一个驱动器 */
#define STATUS_BUSY	0x10		/* FDC busy 软盘控制器忙 */
#define STATUS_DMA	0x20		/* 0- DMA mode DMA数据传输模式 */
#define STATUS_DIR	0x40		/* 0- cpu->fdc 方向， 0:表示cpu到fdc， 1:表示fdc到cpu */
#define STATUS_READY	0x80		/* Data reg ready  数据寄存器就绪位 */

/* Bits of FD_ST0 */
// 状态字节0， ST0 各比特位的含义
#define ST0_DS		0x03		/* drive select mask 驱动器选择号，发生中断时 */
#define ST0_HA		0x04		/* Head (Address)  磁头号 */
#define ST0_NR		0x08		/* Not Ready  磁盘驱动器未准备好 */
#define ST0_ECE		0x10		/* Equipment chech error 设备检测出错（零道校准出错） */
#define ST0_SE		0x20		/* Seek end 寻道或重新校正操作执行结束 */
#define ST0_INTR	0xC0		/* Interrupt code mask  中断代码屏蔽码 */

/* Bits of FD_ST1 */
#define ST1_MAM		0x01		/* Missing Address Mark  为指导地址标志 ID AM */
#define ST1_WP		0x02		/* Write Protect  写保护 */
#define ST1_ND		0x04		/* No Data - unreadable 未找到指定的扇区 */
#define ST1_OR		0x10		/* OverRun  数据传输超时，DMA控制器故障 */
#define ST1_CRC		0x20		/* CRC error in data or addr CRC校验出错 */
#define ST1_EOC		0x80		/* End Of Cylinder 访问超过磁道上的最大扇区号*/

/* Bits of FD_ST2 */
#define ST2_MAM		0x01		/* Missing Addess Mark (again) 未找到数据地址标志 */
#define ST2_BC		0x02		/* Bad Cylinder 磁道坏 */
#define ST2_SNS		0x04		/* Scan Not Satisfied 检索（扫描）条件不满足*/
#define ST2_SEH		0x08		/* Scan Equal Hit 检索条件满足 */
#define ST2_WC		0x10		/* Wrong Cylinder 磁道（柱面号）不符*/
#define ST2_CRC		0x20		/* CRC error in data field CRC校验错 */
#define ST2_CM		0x40		/* Control Mark = deleted 读数据遇到删除标志 */

/* Bits of FD_ST3 */
#define ST3_HA		0x04		/* Head (Address) 磁头号 */
#define ST3_TZ		0x10		/* Track Zero signal (1=track 0)零磁道信号 */
#define ST3_WP		0x40		/* Write Protect 写保护 */

/* Values for FD_COMMAND */
#define FD_RECALIBRATE	0x07		/* move to track 0 重新校正（磁道退道零磁道） */
#define FD_SEEK		0x0F		/* seek track 磁头寻道 */
#define FD_READ		0xE6		/* read with MT, MFM, SKip deleted 读数据（MT 多磁道操作，MFM格式， 跳过删除数据 */
#define FD_WRITE	0xC5		/* write with MT, MFM 写数据 */
#define FD_SENSEI	0x08		/* Sense Interrupt Status 检测中断状态 */
#define FD_SPECIFY	0x03		/* specify HUT etc  设定驱动器参数（步进速率，磁头卸载时间等） */

/* DMA commands */
// DMA读盘，DMA方式字，送DMA断后，12，11
#define DMA_READ	0x46
// DMA写盘，DMA方式字
#define DMA_WRITE	0x4A

#endif
