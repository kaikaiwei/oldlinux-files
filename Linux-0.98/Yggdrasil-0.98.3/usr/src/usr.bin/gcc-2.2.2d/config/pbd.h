/* Definitions of target machine for GNU compiler.

   Citicorp/TTI Unicom PBD version
   (using GAS and  COFF (encapsulated is unacceptable) )

   Copyright (C) 1990 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "sparc.h"

/* Names to predefine in the preprocessor for this target machine.  */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dsparc -DUnicomPBD -Dunix"

/* We want DBX format for use with gdb under COFF.  */

#define DBX_DEBUGGING_INFO

/* Generate calls to memcpy, memcmp and memset.  */

#define TARGET_MEM_FUNCTIONS

/* we use /lib/libp/lib*  when profiling */

#undef LIB_SPEC
#define LIB_SPEC "%{p:-L/usr/lib/libp} %{pg:-L/usr/lib/libp} -lc"


/* Use crt1.o as a startup file and crtn.o as a closing file.  */
/*
 * The loader directive file gcc.ifile defines how to merge the constructor 
 * sections into the data section.  Also, since gas only puts out those 
 * sections in response to N_SETT stabs, and does not (yet) have a 
 * ".sections" directive, gcc.ifile also defines the list symbols 
 * __DTOR_LIST__ and __CTOR_LIST__.
 * 
 * Finally, we must explicitly specify the file from libgcc.a that defines
 * exit(), otherwise if the user specifies (for example) "-lc_s" on the 
 * command line, the wrong exit() will be used and global destructors will 
 * not get called .
 */

#define STARTFILE_SPEC \
"%{!r: gcc.ifile%s} %{pg:gcrt1.o%s}%{!pg:%{p:mcrt1.o%s}%{!p:crt1.o%s}} \
%{!r:_exit.o%s}"

#define ENDFILE_SPEC "crtn.o%s"

/* cpp has to support a #sccs directive for the /usr/include files */

#define SCCS_DIRECTIVE

/* LINK_SPEC is needed only for Sunos 4.  */

#undef LINK_SPEC

/* Although the gas we use can create .ctor and .dtor sections from N_SETT
   stabs, it does not support section directives, so we need to have the loader
   define the lists.
   */
#define CTOR_LISTS_DEFINED_EXTERNALLY

/* similar to default, but allows for the table defined by ld with gcc.ifile. 
   nptrs is always 0.  So we need to instead check that __DTOR_LIST__[1] != 0.
   The old check is left in so that the same macro can be used if and when  
   a future version of gas does support section directives. */

#define DO_GLOBAL_DTORS_BODY {int nptrs = *(int *)__DTOR_LIST__; int i; \
  if (nptrs == -1 || (__DTOR_LIST__[0] == 0 && __DTOR_LIST__[1] != 0))  \
    for (nptrs = 0; __DTOR_LIST__[nptrs + 1] != 0; nptrs++); 		\
  for (i = nptrs; i >= 1; i--)						\
    __DTOR_LIST__[i] (); }

/* 
 * Here is an example gcc.ifile.  I've tested it on PBD sparc
 * systems. The NEXT(0x200000) works on just about all 386 and m68k systems, 
 * but can be reduced to any power of 2 that is >= NBPS (0x40000 on a pbd).

   SECTIONS {
       .text BIND(0x41000200) BLOCK (0x200) : 
		{ *(.init) *(.text) vfork = fork; *(.fini) }

      	GROUP BIND( NEXT(0x200000) + ADDR(.text) + SIZEOF(.text)):
	{      .data : { __CTOR_LIST__ = . ; . += 4; *(.ctor) . += 4 ;
		       	 __DTOR_LIST__ = . ; . += 4; *(.dtor) . += 4 ; }
	       .bss : { }
       }
  }
 */

#ifndef __GNUC__
#define USE_C_ALLOCA
#endif


#undef  ASM_OUTPUT_LABELREF
#define ASM_OUTPUT_LABELREF(FILE,NAME)	\
  fprintf (FILE, "%s", NAME)

/* fixes: */
/*
 *  Internal labels are prefixed with a period.
 */

/* This is how to store into the string LABEL
   the symbol_ref name of an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.
   This is suitable for output with `assemble_name'.  */

#undef ASM_GENERATE_INTERNAL_LABEL

#define ASM_GENERATE_INTERNAL_LABEL(LABEL,PREFIX,NUM)                   \
        sprintf (LABEL, "*.%s%d", PREFIX, NUM)


/* This is how to output an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.  */

#undef  ASM_OUTPUT_INTERNAL_LABEL
#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM)                      \
        fprintf (FILE, ".%s%d:\n", PREFIX, NUM)

/* This is how to output an element of a case-vector that is relative.  */

#undef  ASM_OUTPUT_ADDR_DIFF_ELT
#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, VALUE, REL)  \
  fprintf (FILE, "\t.word .L%d-.L%d\n", VALUE, REL)

/* This is how to output an element of a case-vector that is absolute.
   (The 68000 does not use such vectors,
   but we must define this macro anyway.)  */

#undef ASM_OUTPUT_ADDR_VEC_ELT
#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)  \
  fprintf (FILE, "\t.word .L%d\n", VALUE)

/* Underscores are not used on Unicom PBD coff systems. */
/* This currently seems to only be needed for libgcc2.a */
#define NO_UNDERSCORES

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)  \
  fprintf (FILE, "\tsethi %%hi(.LP%d),%%o0\n\tcall mcount\n\tor %%lo(.LP%d),%%o0,%%o0\n", \
	   (LABELNO), (LABELNO))

/* Output assembler code to FILE to initialize this source file's
   basic block profiling info, if that has not already been done.  */

#undef FUNCTION_BLOCK_PROFILER
#define FUNCTION_BLOCK_PROFILER(FILE, LABELNO)  \
  fprintf (FILE, "\tsethi %%hi(.LPBX0),%%o0\n\tld [%%lo(.LPBX0)+%%o0],%%o1\n\ttst %%o1\n\tbne .LPY%d\n\tnop\n\tcall ___bb_init_func\n\tnop\n.LPY%d:\n",  \
	   (LABELNO), (LABELNO))

/* Output assembler code to FILE to increment the entry-count for
   the BLOCKNO'th basic block in this source file.  */

#undef BLOCK_PROFILER
#define BLOCK_PROFILER(FILE, BLOCKNO) \
{								\
  int blockn = (BLOCKNO);					\
  fprintf (FILE, "\tsethi %%hi(.LPBX2+%d),%%g1\n\tld [%%lo(.LPBX2+%d)+%%g1],%%g2\n\
\tadd %%g2,1,%%g2\n\tst %%g2,[%%lo(.LPBX2+%d)+%%g1]\n",		\
	   4 * blockn, 4 * blockn, 4 * blockn);			\
  CC_STATUS_INIT;  /* We have clobbered %g1.  Also %g2.  */	\
}
/*   This is needed for SunOS 4.0, and should not hurt for 3.2
   versions either.  */
#undef ASM_OUTPUT_SOURCE_LINE(file, line) 
#define ASM_OUTPUT_SOURCE_LINE(file, line)		\
  { static int sym_lineno = 1;				\
    fprintf (file, ".stabn 68,0,%d,.LM%d\n.LM%d:\n",	\
	     line, sym_lineno, sym_lineno);		\
    sym_lineno += 1; }

#define ASM_INT_OP ".long "
