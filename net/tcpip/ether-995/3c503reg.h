/* Definitions for the 3Com 3c503 Etherlink 2. */
/* This file is part of Donald Becker's 8390 drivers.
   This file is distributed under the Linux GPL.
   Some of these names and comments are from the Crynwr packet drivers. */

#define EL2H (dev->base_addr + 0x400)
#define EL2L (dev->base_addr)

/* Shared memory management parameters */

#define EL2SM_START_PG	(0x20)	/* First page of TX buffer */
#define EL2SM_STOP_PG	(0x40)	/* Last page +1 of RX ring */

/* 3Com 3c503 ASIC registers */
#define E33G_STARTPG	(EL2H+0)       /* Start page, must match EN0_STARTPG */
#define E33G_STOPPG	(EL2H+1)	/* Stop  page, must match EN0_STOPPG */
#define E33G_DRQCNT	(EL2H+2)	/* DMA burst count */
#define E33G_IOBASE	(EL2H+3)	/* Read of I/O base jumpers. */
	/* (non-useful, but it also appears at the end of EPROM space) */
#define E33G_ROMBASE	(EL2H+4)	/* Read of memory base jumpers. */
#define E33G_GACFR	(EL2H+5)	/* Config/setup bits for the ASIC GA */
#define E33G_CNTRL	(EL2H+6)	/* Board's main control register */
#define E33G_STATUS	(EL2H+7)	/* Status on completions. */
#define E33G_IDCFR	(EL2H+8)	/* Interrupt/DMA config register */
				/* (Which IRQ to assert, DMA chan to use) */
#define E33G_DMAAH	(EL2H+9)	/* High byte of DMA address reg */
#define E33G_DMAAL	(EL2H+10)	/* Low byte of DMA address reg */
/* "Vector pointer" - if this address matches a read, the EPROM (rather than
   shared RAM) is mapped into memory space. */
#define E33G_VP2	(EL2H+11)
#define E33G_VP1	(EL2H+12)
#define E33G_VP0	(EL2H+13)
#define E33G_FIFOH	(EL2H+14)	/* FIFO for programmed I/O moves */
#define E33G_FIFOL	(EL2H+15)	/* ... low byte of above. */

/* Bits in E33G_CNTRL register: */

#define ECNTRL_RESET	(0x01)	/* Software reset of the ASIC and 8390 */
#define ECNTRL_THIN	(0x02)	/* Onboard thin-net xcvr enable */
#define ECNTRL_SAPROM	(0x04)	/* Map the station address prom */
#define ECNTRL_DBLBFR	(0x20)	/* FIFO configuration bit */
#define ECNTRL_OUTPUT	(0x40)	/* PC-to-3C503 direction if 1 */
#define ECNTRL_INPUT	(0x00)	/* 3C503-to-PC direction if 0 */
#define ECNTRL_START	(0x80)	/* Start the DMA logic */

/* Bits in E33G_STATUS register: */

#define ESTAT_DPRDY	(0x80)	/* Data port (of FIFO) ready */
#define ESTAT_UFLW	(0x40)	/* Tried to read FIFO when it was empty */
#define ESTAT_OFLW	(0x20)	/* Tried to write FIFO when it was full */
#define ESTAT_DTC	(0x10)	/* Terminal Count from PC bus DMA logic */
#define ESTAT_DIP	(0x08)	/* DMA In Progress */

/* Bits in E33G_GACFR register: */

#define EGACFR_NORM	(0x49)	/* Enable 8K shared mem, no DMA TC int */
#define EGACFR_IRQOFF	(0xc9)	/* Above, and disable 8390 IRQ line */

/* End of 3C503 parameter definitions */
