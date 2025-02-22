/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
   Derived from @(#)_setjmp.s	5.7 (Berkeley) 6/27/88,
   Copyright (c) 1980 Regents of the University of California.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <ansidecl.h>
#include <setjmp.h>

#ifndef	__GNUC__
  #error This file uses GNU C extensions; you must compile with GCC.
#endif


#define	REI	02	/* Vax `rei' opcode.  */

/* Jump to the position specified by ENV, causing the
   setjmp call there to return VAL, or 1 if VAL is 0.  */
__NORETURN
void
DEFUN(__longjmp, (env, val), CONST jmp_buf env AND int val)
{
  register long int *fp asm("fp");
  long int *regsave;
  unsigned long int flags;

  if (env.__fp == NULL)
    __libc_fatal("longjmp: Invalid ENV argument.\n");

  if (val == 0)
    val = 1;

  asm volatile("loop:");

  flags = *(long int *) (6 + (char *) fp);
  regsave = (long int *) (20 + (char *) fp);
  if (flags & 1)
    /* R0 was saved by the caller.
       Store VAL where it will be restored from.  */
    *regsave++ = val;
  if (flags & 2)
    /* R1 was saved by the caller.
       Store ENV where it will be restored from.  */
    *regsave = env;

  /* Was the FP saved in the last call the same one in ENV?  */
  asm volatile("cmpl %0, 12(fp);"
	       /* Yes, return to it.  */
	       "beql done;"
	       /* The FP in ENV is less than the one saved in the last call.
		  This means we have already returned from the function that
		  called `setjmp' with ENV!  */
	       "blssu latejump;" : /* No outputs.  */ : "g" (env.__fp));

  /* We are more than one level below the state in ENV.
     Return to where we will pop another stack frame.  */
  asm volatile("movl $loop, 16(fp);"
	       "ret");

  asm volatile("done:");
  {
    char return_insn asm("*16(fp)");
    if (return_insn == REI)
      /* We're returning with an `rei' instruction.
	 Do a return with PSL-PC pop.  */
      asm volatile("movab 0f, 16(fp)");
    else
      /* Do a standard return.  */
      asm volatile("movab 1f, 16(fp)");

    /* Return.  */
    asm volatile("ret");
  }

  asm volatile("0:"	/* `rei' return.  */
	       /* Compensate for PSL-PC push.  */
	       "addl2 %0, sp;"
	       "1:"	/* Standard return.  */
	       /* Return to saved PC.  */
	       "jmp %1" : /* No outputs.  */ :
	       "g" (8), "g" (env.__pc));

  /* Jump here when the FP saved in ENV points
     to a function that has already returned.  */
  asm volatile("latejump:");
  __libc_fatal("longjmp: Attempt to jump to a function that has returned.\n");
}
