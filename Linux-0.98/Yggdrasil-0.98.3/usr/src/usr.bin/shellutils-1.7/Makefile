# Generated automatically from Makefile.in by configure.
# Master Makefile for the GNU shell programming utilities.
# Copyright (C) 1991 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#### Start of system configuration section. ####

srcdir = .
VPATH = .

# If you use gcc, you should either run the fixincludes script that
# comes with it or else use gcc with the -traditional option.  Otherwise
# ioctl calls will be compiled incorrectly on some systems.
CC = gcc -O
AR = ar
# Set RANLIB = @: if your system doesn't have or need ranlib.
RANLIB = ranlib
# Use cp if you don't have install.
INSTALL = /bin/install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

# Things you might add to DEFS:
# -DSTDC_HEADERS	If you have ANSI C headers and libraries.
# -DHAVE_UNISTD_H	If you have unistd.h.
# -DHAVE_LIMITS_H	If you have limits.h.
# -DUSG			If you have System V/ANSI C string and
#			memory functions and headers.
# -DHAVE_SYSLOG_H	If you have syslog.h (and the rest of syslog).
# -DHAVE_SHADOW_H	If you have shadow.h (SysV shadow passwords).
# -DHAVE_STRERROR	If you have strerror function.
# -DHAVE_VPRINTF	If you have vprintf function.
# -DHAVE_DOPRNT		If you have _doprnt function (but lack vprintf).
# -DHAVE_FTIME		If you have ftime system call.
# -DHAVE_UNAME		If you have uname system call (but lack gethostname).
# -DTM_IN_SYS_TIME	If you need to include sys/time.h instead of
#			time.h to get struct tm.
# -DHAVE_TM_ZONE	If you have tm_zone in struct tm; otherwise
#			uses tm_isdst and tzname.
# -DHAVE_TZNAME		If you have tm_zone and tzname but not tm_zone;
#			otherwise uses tm_isdst and timezone.
# -DHAVE_C_LINE		If you have struct termios with the c_line member.
# -DHAVE_UT_HOST	If you have ut_host in struct utmp.
# -DNICE_PRIORITY	If you lack getpriority and setpriority system
#			calls but have nice system call.
# -DWINSIZE_IN_PTEM	If your system defines `struct winsize' in sys/ptem.h.
# -DGWINSZ_BROKEN	If TIOCGWINSZ is defined but doesn't work.

DEFS =  -DHAVE_LIMITS_H=1 -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DHAVE_STRERROR=1 -DHAVE_FTIME=1 -DHAVE_UT_HOST=1 -DHAVE_VPRINTF=1 -DHAVE_ALLOCA_H=1 -DDIRENT=1

CFLAGS = -g
LDFLAGS = -g
LIBS = 

prefix = /usr/local
exec_prefix = $(prefix)

# Prefix for each installed program, normally empty or `g'.
binprefix = 
# Prefix for each installed man page, normally empty or `g'.
manprefix = 

# Where to install the executables.
bindir = $(exec_prefix)/bin

# Where to install the manual pages.
mandir = $(prefix)/man/man1
# Extension (not including `.') for the installed manual page filenames.
manext = 1

#### End of system configuration section. ####

SHELL = /bin/sh

MDEFINES = bindir='$(bindir)' mandir='$(mandir)' manext='$(manext)' \
binprefix='$(binprefix)' manprefix='$(manprefix)' \
LIBS='$(LIBS)' AR='$(AR)' RANLIB='$(RANLIB)' \
INSTALL_PROGRAM='$(INSTALL_PROGRAM)' INSTALL_DATA='$(INSTALL_DATA)' \
DEFS='$(DEFS)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' \
LIBS='$(LIBS)' CC='$(CC)'

DISTFILES = COPYING COPYING.LIB ChangeLog Makefile.in README NEWS INSTALL \
configure configure.in

# Redundant stuff for making only selected programs.
PROGS = basename date dirname env expr false groups id logname \
pathchk printenv printf sleep su tee test true tty who whoami yes \
 uname nice nohup

# Subdirectories to run make in for the primary targets.
SUBDIRS = lib src man

all:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
.PHONY: all

$(PROGS):
	cd lib; $(MAKE) $(MDEFINES) all
	cd src; $(MAKE) $(MDEFINES) $@

install:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
.PHONY: install

TAGS:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
.PHONY: TAGS

clean:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
.PHONY: clean

mostlyclean:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
.PHONY: mostlyclean

distclean:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
	rm -f Makefile config.status
.PHONY: distclean

realclean:
	for dir in $(SUBDIRS); do echo making $@ in $$dir; cd $$dir; $(MAKE) $(MDEFINES) $@; cd ..; done
	rm -f Makefile config.status
.PHONY: realclean

dist:
	echo shellutils-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q src/version.c` > .fname
	rm -rf `cat .fname`
	mkdir `cat .fname`
	ln $(DISTFILES) `cat .fname`
	for dir in $(SUBDIRS); do mkdir `cat .fname`/$$dir; cd $$dir; $(MAKE) $@; cd ..; done
	tar chZf `cat .fname`.tar.Z `cat .fname`
	rm -rf `cat .fname` .fname
.PHONY: dist

# Prevent GNU make v3 from overflowing arg limit on SysV.
.NOEXPORT:
