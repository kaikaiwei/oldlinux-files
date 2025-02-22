# Generated automatically from Makefile.in by configure.
# Copyright (C) 1989, 1990, 1991, 1992 Free Software Foundation, Inc.
#      Written by James Clark (jjc@jclark.com)
# 
# This file is part of groff.
# 
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.
# 
# groff is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with groff; see the file COPYING.  If not, write to the Free Software
# Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

# Define `page' to be letter if your PostScript printer uses 8.5x11
# paper (USA) and define it to be A4, if it uses A4 paper (rest of the
# world).
PAGE=letter

# Normally the Postscript driver, grops, produces output that conforms
# to version 3.0 of the Adobe Document Structuring Conventions.
# Unfortunately some spoolers and previewers can't handle such output.
# The BROKEN_SPOOLER_FLAGS variable tells grops what it should do to
# make its output acceptable to such programs.  This variable controls
# only the default behaviour of grops; the behaviour can be changed at
# runtime by the grops -b option (and so by groff -P-b).
# Use a value of 0 if your spoolers and previewers are able to handle
# conforming PostScript correctly.
# Add 1 if no %%{Begin,End}DocumentSetup comments should be generated;
# this is needed for early versions of TranScript that get confused by
# anything between the %%EndProlog line and the first %%Page: comment.
# Add 2 if lines in included files beginning with %! should be
# stripped out; this is needed for the OpenWindows 2.0 pageview previewer.
# Add 4 if %%Page, %%Trailer and %%EndProlog comments should be
# stripped out of included files; this is needed for spoolers that
# don't understand the %%{Begin,End}Document comments. I suspect this
# includes early versions of TranScript.
BROKEN_SPOOLER_FLAGS=7

# DEVICE is the default device.
DEVICE=ps

# PSPRINT is the command to use for printing a PostScript file,
# for example `lpr'.
PSPRINT=

# DVIPRINT is the command to use for printing a TeX dvi file,
# for example `lpr -d'.
DVIPRINT=

# Prefix for names of programs that have Unix counterparts.
# For example, if `g' is `g' then troff will be installed as
# gtroff.  This doesn't affect programs like grops or groff that have
# no Unix counterparts.  Note that the groff versions of eqn and tbl
# will not work with Unix troff.  This is also use in the definitions
# of tmac_s and tmac_m.
g=g

# The groff ms macros will be available as -m$(tmac_s).
# If you use `tmac_s=s', you can use the Unix ms macros by using
# groff -ms -M/usr/lib/tmac.
tmac_s=$(g)s

# Similarily, the groff mm macros will be available as -m$(tmac_m).
tmac_m=$(g)m

# Common prefix for installation directories.
# Used in definitions of exec_prefix, datasubdir, fontpath, manroot.
# This must already exist when you do make install.
prefix = /usr

exec_prefix = $(prefix)

# bindir says where to install executables.
bindir=$(exec_prefix)/bin

# datasubdir says where to install executables
datadir=$(prefix)/lib
datasubdir=$(datadir)/groff

# fontdir says where to install dev*/*.
fontdir=$(datasubdir)/font

# fontpath says where to look for dev*/*.
fontpath=.:$(fontdir):$(prefix)/lib/font:/usr/lib/font

# tmacdir says where to install macros.
tmacdir=$(datasubdir)/tmac

# tmacpath says where to look for macro files.
tmacpath=.:$(tmacdir):/usr/lib/tmac

# hyphenfile is the file containing the hyphenation patterns.
hyphenfile=$(datasubdir)/hyphen

# Extension to be used for refer index files.  Index files are not
# shareable between different architectures, so you might want to use
# different suffixes for different architectures.  Choose an extension
# that doesn't conflict with refer or any other indexing program.
indexext=.i

# Directory containing the default index for refer.
indexdir=/usr/dict/papers

# The filename (without suffix) of the default index for refer.
indexname=Ind

# common_words_file is a file containing a list of common words.
# If your system provides /usr/lib/eign it will be copied onto this,
# otherwise the supplied eign file will be used.
common_words_file=$(datasubdir)/eign

# manroot is the root of the man page directory tree.
manroot=$(prefix)/man

# man1ext is the man section for user commands.
man1ext=1
man1dir=$(manroot)/man$(man1ext)

# man5ext is the man section for file formats.
man5ext=5
man5dir=$(manroot)/man$(man5ext)

# man7ext is the man section for macros.
man7ext=7
man7dir=$(manroot)/man$(man7ext)

# DEFINES should include the following:
# -DHAVE_MMAP			if you have mmap() and <sys/mman.h>
# -DCFRONT_ANSI_BUG		if you're using cfront 2.0 (or later?) with
#				an ANSI C compiler
# -DCOOKIE_BUG			if you're using gcc/g++ 2.[012] but haven't
#				applied the patch in gcc.patch.
# -DARRAY_DELETE_NEEDS_SIZE	if your C++ doesn't understand `delete []'
# -DHAVE_SYS_SIGLIST		if you have sys_siglist[]
# -DWCOREFLAG=0200		if the 0200 bit of the status returned by
#				wait() indicates whether a core image was
#				produced for a process that was terminated
#				by a signal
# -DHAVE_UNISTD_H		if you have <unistd.h>
# -DHAVE_CC_UNISTD_H		if you have a C++ <unistd.h>
# -DHAVE_DIRENT_H		if you have <dirent.h>
# -DHAVE_LIMITS_H		if you have <limits.h>
# -DHAVE_CC_LIMITS_H		if you have a C++ <limits.h>
# -DHAVE_SYS_DIR_H		if you have <sys/dir.h>
# -DHAVE_VFORK_H		if you have <vfork.h>
# -Dvfork=fork			if you don't have a working vfork()
# -DHAVE_RENAME			if you have rename()
# -DHAVE_MKSTEMP		if you have mkstemp()
# -DSTDLIB_H_DECLARES_GETOPT	if your C++ <stdlib.h> declares getopt()
# -DSTDLIB_H_DECLARES_PUTENV	if your C++ <stdlib.h> declares putenv()
# -DTRADITIONAL_CPP		if your C++ compiler uses a traditional
#				(Reiser) preprocessor.
# -DLONG_FOR_TIME_T		if localtime() takes a long * not a time_t *
DEFINES= -DHAVE_UNISTD_H=1 -DHAVE_DIRENT_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_DIR_H=1 -DHAVE_CC_LIMITS_H=1 -DHAVE_CC_UNISTD_H=1 -DSTDLIB_H_DECLARES_GETOPT=1 -DSTDLIB_H_DECLARES_PUTENV=1 -DHAVE_MMAP=1 -DHAVE_RENAME=1 -DHAVE_SYS_SIGLIST=1 -DCOOKIE_BUG=1

# Include fmod.o, strtol.o, getcwd.o, strerror.o in LIBOBJS if your C library
# is missing the corresponding function.
LIBOBJS=

# CCC is the compiler for C++ (.cc) files.
CCC=gcc -O
CC=gcc -O
# CCDEFINES are definitions for C++ compilations.
CCDEFINES=$(DEFINES)
# CDEFINES are definitions for C compilations.
CDEFINES=$(DEFINES)

DEBUG=-g
#CCFLAGS=$(DEBUG)
#CFLAGS=$(DEBUG)
YACC=bison -y
YACCFLAGS=-v

LIBS=
CCLIBS=
RANLIB=ranlib
INSTALL=/bin/install -c
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644
LN_S=ln -s
AR=ar
ETAGS=etags
ETAGSFLAGS=
# Flag that tells etags to assume C++.
ETAGSCCFLAG=
# Full path to perl.
PERLPATH=

srcdir = .
VPATH = .

MDEFINES="PAGE=$(PAGE)" "BROKEN_SPOOLER_FLAGS=$(BROKEN_SPOOLER_FLAGS)" \
  "DEVICE=$(DEVICE)" "PSPRINT=$(PSPRINT)" "DVIPRINT=$(DVIPRINT)" \
  "prefix=$(prefix)" "exec_prefix=$(exec_prefix)" "bindir=$(bindir)" \
  "g=$(g)" "datadir=$(datadir)" "datasubdir=$(datasubdir)" \
  "fontdir=$(fontdir)" "fontpath=$(fontpath)" \
  "tmacdir=$(tmacdir)" "tmacpath=$(tmacpath)" \
  "hyphenfile=$(hyphenfile)" "indexext=$(indexext)" "indexdir=$(indexdir)" \
  "indexname=$(indexname)" "common_words_file=$(common_words_file)" \
  "manroot=$(manroot)" "man1ext=$(man1ext)" "man1dir=$(man1dir)" \
  "man5ext=$(man5ext)" "man5dir=$(man5dir)" \
  "man7ext=$(man7ext)" "man7dir=$(man7dir)" \
  "tmac_s=$(tmac_s)" "tmac_m=$(tmac_m)" \
  "CCC=$(CCC)" "CC=$(CC)" "CCDEFINES=$(CCDEFINES)" "CDEFINES=$(CDEFINES)" \
  "CCFLAGS=$(CCFLAGS)" "CFLAGS=$(CFLAGS)" \
  "YACC=$(YACC)" "YACCFLAGS=$(YACCFLAGS)" \
  "LIBS=$(LIBS)" "CCLIBS=$(CCLIBS)" "LIBOBJS=$(LIBOBJS)" \
  "RANLIB=$(RANLIB)" "AR=$(AR)" \
  "INSTALL_PROGRAM=$(INSTALL_PROGRAM)" "INSTALL_DATA=$(INSTALL_DATA)" \
  "ETAGS=$(ETAGS)" "ETAGSFLAGS=$(ETAGSFLAGS)" "ETAGSCCFLAG=$(ETAGSCCFLAG)" \
  "PERLPATH=$(PERLPATH)" 

SHELL=/bin/sh
INCDIRS=include
LIBDIRS=libgroff libdriver libbib
CCPROGDIRS=groff troff tbl pic eqn grops grotty grodvi tfmtodit \
  refer lookbib indxbib lkbib soelim addftinfo
CPROGDIRS=pfbtops psbb
PROGDIRS=$(CCPROGDIRS) $(CPROGDIRS)
DEVDIRS=devps devdvi devascii devlatin1
OTHERDIRS=man tmac afmtodit grog nroff mm
ALLDIRS=$(INCDIRS) $(LIBDIRS) $(PROGDIRS) $(DEVDIRS) $(OTHERDIRS)
EXTRADIRS=devps/generate devdvi/generate xditview doc
DISTDIRS=$(ALLDIRS) $(EXTRADIRS)
TARGETS=all install install_bin install_data clean distclean mostlyclean \
  realclean extraclean distfiles TAGS depend

do=all
dodirs=$(ALLDIRS) dot
# Default target for subdir_Makefile
subdir=troff


$(TARGETS):
	@$(MAKE) do=$@ $(dodirs)

dot: FORCE
	@$(MAKE) srcdir=$(srcdir) VPATH=$(srcdir) -f $(srcdir)/Makefile.comm \
		-f $(srcdir)/Makefile.sub $(do)
	
$(LIBDIRS): FORCE
	@if test $(srcdir) = .; \
	then srcdir=.; \
	else srcdir=`cd $(srcdir); pwd`/$@; \
	fi; \
	echo Making $(do) in $@; \
	test -d $@ || mkdir $@; \
	cd $@; \
	$(MAKE) $(MDEFINES) srcdir=$$srcdir VPATH=$$srcdir \
		-f $$srcdir/../Makefile.comm -f $$srcdir/Makefile.sub \
	        -f $$srcdir/../Makefile.lib -f $$srcdir/Makefile.dep $(do)

$(CPROGDIRS): FORCE
	@if test $(srcdir) = .; \
	then srcdir=.; \
	else srcdir=`cd $(srcdir); pwd`/$@; \
	fi; \
	echo Making $(do) in $@; \
	test -d $@ || mkdir $@; \
	cd $@; \
	$(MAKE) $(MDEFINES) srcdir=$$srcdir VPATH=$$srcdir \
		-f $$srcdir/../Makefile.comm -f $$srcdir/Makefile.sub \
	        -f $$srcdir/../Makefile.cpg -f $$srcdir/Makefile.dep $(do)

$(CCPROGDIRS): FORCE
	@if test $(srcdir) = .; \
	then srcdir=.; \
	else srcdir=`cd $(srcdir); pwd`/$@; \
	fi; \
	echo Making $(do) in $@; \
	test -d $@ || mkdir $@; \
	cd $@; \
	$(MAKE) $(MDEFINES) srcdir=$$srcdir VPATH=$$srcdir \
		-f $$srcdir/../Makefile.comm -f $$srcdir/Makefile.sub \
	        -f $$srcdir/../Makefile.ccpg -f $$srcdir/Makefile.dep $(do)

$(DEVDIRS): FORCE
	@if test $(srcdir) = .; \
	then srcdir=.; \
	else srcdir=`cd $(srcdir); pwd`/$@; \
	fi; \
	echo Making $(do) in $@; \
	test -d $@ || mkdir $@; \
	cd $@; \
	$(MAKE) $(MDEFINES) srcdir=$$srcdir VPATH=$$srcdir \
		-f $$srcdir/../Makefile.comm -f $$srcdir/Makefile.sub \
		-f $$srcdir/../Makefile.dev $(do)

$(INCDIRS) $(OTHERDIRS): FORCE
	@if test $(srcdir) = .; \
	then srcdir=.; \
	else srcdir=`cd $(srcdir); pwd`/$@; \
	fi; \
	echo Making $(do) in $@; \
	test -d $@ || mkdir $@; \
	cd $@; \
	$(MAKE) $(MDEFINES) srcdir=$$srcdir VPATH=$$srcdir \
		-f $$srcdir/../Makefile.comm -f $$srcdir/Makefile.sub \
	        -f $$srcdir/../Makefile.man $(do)

version=`cat $(srcdir)/VERSION`

.PHONY: dist
dist:
	-rm -fr tmp
	mkdir tmp
	for d in $(DISTDIRS); do \
	  mkdir	tmp/$$d; \
	done
	srcdir=`cd $(srcdir); pwd`; \
	cd tmp; \
	$(LN_S) $$srcdir/* . 2>/dev/null; \
	for d in $(DISTDIRS); do \
	  (cd $$d; $(LN_S) $$srcdir/$$d/* . 2>/dev/null); \
	done; \
	$(MAKE) -f ../Makefile srcdir=$$srcdir VPATH=$$srcdir extraclean; \
	for d in $(EXTRADIRS); do \
	  (cd $$d; $(MAKE) extraclean); \
	done; \
	$(LN_S) $$srcdir/Makefile.init Makefile
	mv tmp groff-$(version)
	tar cfh - groff-$(version) | compress >groff-$(version).tar.Z
	rm -fr groff-$(version)

# $(PROGDIRS): libgroff
# grops grotty grodvi: libdriver
# refer lookbib indxbib lkbib: libbib
# $(LIBDIRS) $(PROGDIRS): include

.PHONY: $(ALLDIRS) dot $(TARGETS) FORCE

subdir_Makefile: Makefile.cfg
	$(MAKE) do=Makefile $(subdir)

Makefile.cfg: Makefile
	>Makefile.cfg
	for var in $(MDEFINES); do \
	  echo "$$var" >>Makefile.cfg; \
	done

depend: srcdir_must_be_dot

.PHONY: srcdir_must_be_dot
srcdir_must_be_dot:
	@test "X$(srcdir)" = "X." \
	  || (echo This target can only be made in the source directory; \
	      exit 1)

Makefile: Makefile.in
	$(SHELL) config.status

FORCE:

.NOEXPORT:
