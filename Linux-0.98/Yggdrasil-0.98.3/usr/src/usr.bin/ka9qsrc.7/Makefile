#
#	Makefile for KA9Q TCP/IP package for UNIX System V
#

SHELL = /bin/sh
TAR = tar
LINT = lint
INSTALL = /usr/local/bin

#
# Add -DXOBBS to the CFLAGS line of choice if you wish to include the W2XO
# PBBS code for sysV Unix... and add -DSID2 if you want a separate 
# 'ax25 bbscall' callsign/ssid for the BBS port.
#

#
# Uncomment the next five lines for
# Microport System V release 3 -- 80386 with shared libraries.
#
CFLAGS = -DUNIX -DSYS5 -DSLOWCHECK -DLITTLE_ENDIAN -DMSPTICK=10 -O6 -fomit-frame-pointer
LINTFLAGS = -DUNIX -DSYS5 -DSLOWCHECK -DLITTLE_ENDIAN -DMSPTICK=10
LIBS = -static
########################################################################
#
# Uncomment the next five lines for
# Microport System V -- 80286 without shared libraries.
#
#CFLAGS = -g -Ml -O -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17
#LINTFLAGS = -g -Ml -O -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17
#LIBS =
########################################################################
#
# Uncomment the next five lines for
# Unix System V -- AT&T 3B2
#
#CFLAGS = -O -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=10
#LINTFLAGS = -O -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=10
#LIBS =
########################################################################
#
# Uncomment the next three lines for
# Unix System V -- Motorola 680x0 
#
#CFLAGS = -g -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17 -DXOBBS -DSID2
#LINTFLAGS = -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17 -DXOBBS -DSID2
#LIBS =
########################################################################
#
# Uncomment the next four lines for
# Sun OS 4.x, Sys V environment
#
#CFLAGS = -g -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17 -DSUNOS4
#LINTFLAGS = -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17 -DSUNOS4
#LINT = /usr/5bin/lint
#LIBS = /usr/5lib/libc.a
########################################################################
#
# Uncomment the next three lines for
# HP9000/500 - HP-UX 5.21 (not confident MSPTICK is correct!)
#
#CFLAGS =    -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17 -DXOBBS -DSID2
#LINTFLAGS = -DUNIX -DSYS5 -DSLOWCHECK -DMSPTICK=17 -DXOBBS -DSID2
#LIBS =
########################################################################

SRCS =	main.c arp.c arpcmd.c arpdump.c ax25.c ax25cmd.c \
	ax25dump.c ax25subr.c ax25user.c ax_mbx.c bootp.c cmdparse.c daemon.c \
	domain.c files.c fingcli.c fingserv.c ftp.c ftpcli.c ftpserv.c icmp.c \
	icmpcmd.c icmpdump.c icmpmsg.c iface.c ip.c ipcmd.c ipdump.c \
	iproute.c kiss.c lapb.c lapbtime.c lcsum.c mbuf.c misc.c mulport.c \
	ndir.c netuser.c nrcmd.c nrs.c nr3.c nrsubr.c nr4.c nr4subr.c \
	nr4user.c nr4timer.c pathname.c session.c slfp.c slip.c smisc.c slhc.c \
	smtpcli.c smtpserv.c sys5_io.c sys5unix.c tcpcmd.c tcpdump.c rdate.c \
	tcpin.c tcpout.c tcpsubr.c tcptimer.c tcpuser.c telnet.c \
	telunix.c timer.c tnserv.c trace.c ttydriv.c udp.c udpcmd.c \
	udpdump.c version.c wildmat.c linux.c enet.c enetdump.c xserv.c

OBJS =	main.o arp.o arpcmd.o arpdump.o ax25.o ax25cmd.o \
	ax25dump.o ax25subr.o ax25user.o ax_mbx.o bootp.o cmdparse.o daemon.o \
	domain.o files.o fingcli.o fingserv.o ftp.o ftpcli.o ftpserv.o icmp.o \
	icmpcmd.o icmpdump.o icmpmsg.o iface.o ip.o ipcmd.o ipdump.o \
	iproute.o kiss.o lapb.o lapbtime.o lcsum.o mbuf.o misc.o mulport.o \
	netuser.o nrcmd.o nrs.o nr3.o nrsubr.o nr4.o nr4subr.o \
	nr4user.o nr4timer.o pathname.o session.o slfp.o slip.o slhc.o smisc.o \
	smtpcli.o smtpserv.o sys5_io.o sys5unix.o tcpcmd.o tcpdump.o rdate.o \
	tcpin.o tcpout.o tcpsubr.o tcptimer.o tcpuser.o telnet.o \
	telunix.o timer.o tnserv.o trace.o ttydriv.o udp.o udpcmd.o \
	udpdump.o version.o wildmat.o linux.o enet.o enetdump.o xserv.o

default: net

net:	$(OBJS)
	$(CC) $(CFLAGS) -o net $(OBJS) $(LIBS)
#	cp net.debug net
#	strip net

tar:
	tar -cz -f /work/ka9qsrc.tar.Z -X src.excludes .
	tar -cz -f /work/ka9qbin.tar.Z net READ.ME startup.net startup.eth \
		README.WD we.c wereg.h we.diffs

lint:	$(SRCS)
	rm -f *.ln
	$(LINT) -c $(LINTFLAGS) $(SRCS) > lint.out 2>&1
	$(LINT) $(LINTFLAGS) *.ln > lint2.out 2>&1
	rm *.ln

depend: 
	$(CC) -M $(LINTFLAGS) $(SRCS) > depend

test_tick:	test_tick.c
	$(CC) -o test_tick test_tick.c $(LIBS)

clean:
	rm -f *.o *.ln lint.out lint2.out net net.debug test_tick
xtar:
	$(TAR) cvf svnet.tar make* mkdep depend.out hosts.net ftpusers \
		startup.net *.[hc] *.orig *.ori
	compress -f svnet.tar

include depend.out

