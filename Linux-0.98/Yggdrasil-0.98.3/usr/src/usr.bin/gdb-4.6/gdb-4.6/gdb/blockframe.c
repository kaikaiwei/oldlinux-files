/* Get info from stack frames;
   convert between frames, blocks, functions and pc values.
   Copyright 1986, 1987, 1988, 1989, 1991 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "defs.h"
#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "frame.h"
#include "gdbcore.h"
#include "value.h"		/* for read_register */
#include "target.h"		/* for target_has_stack */
#include "inferior.h"		/* for read_pc */

/* Is ADDR inside the startup file?  Note that if your machine
   has a way to detect the bottom of the stack, there is no need
   to call this function from FRAME_CHAIN_VALID; the reason for
   doing so is that some machines have no way of detecting bottom
   of stack.  */

int
inside_entry_file (addr)
     CORE_ADDR addr;
{
  if (symfile_objfile == 0)
    return 0;
  return (addr >= symfile_objfile -> ei.entry_file_lowpc &&
	  addr <  symfile_objfile -> ei.entry_file_highpc);
}

/* Test a specified PC value to see if it is in the range of addresses
   that correspond to the main() function.  See comments above for why
   we might want to do this.

   Typically called from FRAME_CHAIN_VALID. */

int
inside_main_func (pc)
CORE_ADDR pc;
{
  if (symfile_objfile == 0)
    return 0;
  return (symfile_objfile -> ei.main_func_lowpc  <= pc &&
	  symfile_objfile -> ei.main_func_highpc > pc);
}

/* Test a specified PC value to see if it is in the range of addresses
   that correspond to the process entry point function.  See comments
   in objfiles.h for why we might want to do this.

   Typically called from FRAME_CHAIN_VALID. */

int
inside_entry_func (pc)
CORE_ADDR pc;
{
  if (symfile_objfile == 0)
    return 0;
  return (symfile_objfile -> ei.entry_func_lowpc  <= pc &&
	  symfile_objfile -> ei.entry_func_highpc > pc);
}

/* Address of innermost stack frame (contents of FP register) */

static FRAME current_frame;

/*
 * Cache for frame addresses already read by gdb.  Valid only while
 * inferior is stopped.  Control variables for the frame cache should
 * be local to this module.
 */
struct obstack frame_cache_obstack;

/* Return the innermost (currently executing) stack frame.  */

FRAME
get_current_frame ()
{
  /* We assume its address is kept in a general register;
     param.h says which register.  */

  return current_frame;
}

void
set_current_frame (frame)
     FRAME frame;
{
  current_frame = frame;
}

FRAME
create_new_frame (addr, pc)
     FRAME_ADDR addr;
     CORE_ADDR pc;
{
  struct frame_info *fci;	/* Same type as FRAME */

  fci = (struct frame_info *)
    obstack_alloc (&frame_cache_obstack,
		   sizeof (struct frame_info));

  /* Arbitrary frame */
  fci->next = (struct frame_info *) 0;
  fci->prev = (struct frame_info *) 0;
  fci->frame = addr;
  fci->next_frame = 0;		/* Since arbitrary */
  fci->pc = pc;

#ifdef INIT_EXTRA_FRAME_INFO
  INIT_EXTRA_FRAME_INFO (0, fci);
#endif

  return fci;
}

/* Return the frame that called FRAME.
   If FRAME is the original frame (it has no caller), return 0.  */

FRAME
get_prev_frame (frame)
     FRAME frame;
{
  /* We're allowed to know that FRAME and "struct frame_info *" are
     the same */
  return get_prev_frame_info (frame);
}

/* Return the frame that FRAME calls (0 if FRAME is the innermost
   frame).  */

FRAME
get_next_frame (frame)
     FRAME frame;
{
  /* We're allowed to know that FRAME and "struct frame_info *" are
     the same */
  return frame->next;
}

/*
 * Flush the entire frame cache.
 */
void
flush_cached_frames ()
{
  /* Since we can't really be sure what the first object allocated was */
  obstack_free (&frame_cache_obstack, 0);
  obstack_init (&frame_cache_obstack);

  current_frame = (struct frame_info *) 0; /* Invalidate cache */
}

/* Flush the frame cache, and start a new one if necessary.  */
void
reinit_frame_cache ()
{
  FRAME fr = current_frame;
  flush_cached_frames ();
  if (fr)
    set_current_frame ( create_new_frame (read_register (FP_REGNUM),
					  read_pc ()));
}

/* Return a structure containing various interesting information
   about a specified stack frame.  */
/* How do I justify including this function?  Well, the FRAME
   identifier format has gone through several changes recently, and
   it's not completely inconceivable that it could happen again.  If
   it does, have this routine around will help */

struct frame_info *
get_frame_info (frame)
     FRAME frame;
{
  return frame;
}

/* If a machine allows frameless functions, it should define a macro
   FRAMELESS_FUNCTION_INVOCATION(FI, FRAMELESS) in param.h.  FI is the struct
   frame_info for the frame, and FRAMELESS should be set to nonzero
   if it represents a frameless function invocation.  */

/* Return nonzero if the function for this frame has a prologue.  Many
   machines can define FRAMELESS_FUNCTION_INVOCATION to just call this
   function.  */

int
frameless_look_for_prologue (frame)
     FRAME frame;
{
  CORE_ADDR func_start, after_prologue;
  func_start = (get_pc_function_start (frame->pc) +
		FUNCTION_START_OFFSET);
  if (func_start)
    {
      after_prologue = func_start;
#ifdef SKIP_PROLOGUE_FRAMELESS_P
      /* This is faster, since only care whether there *is* a prologue,
	 not how long it is.  */
      SKIP_PROLOGUE_FRAMELESS_P (after_prologue);
#else
      SKIP_PROLOGUE (after_prologue);
#endif
      return after_prologue == func_start;
    }
  else
    /* If we can't find the start of the function, we don't really
       know whether the function is frameless, but we should be able
       to get a reasonable (i.e. best we can do under the
       circumstances) backtrace by saying that it isn't.  */
    return 0;
}

/* Default a few macros that people seldom redefine.  */

#if !defined (INIT_FRAME_PC)
#define INIT_FRAME_PC(fromleaf, prev) \
  prev->pc = (fromleaf ? SAVED_PC_AFTER_CALL (prev->next) : \
	      prev->next ? FRAME_SAVED_PC (prev->next) : read_pc ());
#endif

#ifndef FRAME_CHAIN_COMBINE
#define	FRAME_CHAIN_COMBINE(chain, thisframe) (chain)
#endif

/* Return a structure containing various interesting information
   about the frame that called NEXT_FRAME.  Returns NULL
   if there is no such frame.  */

struct frame_info *
get_prev_frame_info (next_frame)
     FRAME next_frame;
{
  FRAME_ADDR address;
  struct frame_info *prev;
  int fromleaf = 0;

  /* If the requested entry is in the cache, return it.
     Otherwise, figure out what the address should be for the entry
     we're about to add to the cache. */

  if (!next_frame)
    {
      if (!current_frame)
	{
	  error ("You haven't set up a process's stack to examine.");
	}

      return current_frame;
    }

  /* If we have the prev one, return it */
  if (next_frame->prev)
    return next_frame->prev;

  /* On some machines it is possible to call a function without
     setting up a stack frame for it.  On these machines, we
     define this macro to take two args; a frameinfo pointer
     identifying a frame and a variable to set or clear if it is
     or isn't leafless.  */
#ifdef FRAMELESS_FUNCTION_INVOCATION
  /* Still don't want to worry about this except on the innermost
     frame.  This macro will set FROMLEAF if NEXT_FRAME is a
     frameless function invocation.  */
  if (!(next_frame->next))
    {
      FRAMELESS_FUNCTION_INVOCATION (next_frame, fromleaf);
      if (fromleaf)
	address = next_frame->frame;
    }
#endif

  if (!fromleaf)
    {
      /* Two macros defined in tm.h specify the machine-dependent
	 actions to be performed here.
	 First, get the frame's chain-pointer.
	 If that is zero, the frame is the outermost frame or a leaf
	 called by the outermost frame.  This means that if start
	 calls main without a frame, we'll return 0 (which is fine
	 anyway).

	 Nope; there's a problem.  This also returns when the current
	 routine is a leaf of main.  This is unacceptable.  We move
	 this to after the ffi test; I'd rather have backtraces from
	 start go curfluy than have an abort called from main not show
	 main.  */
      address = FRAME_CHAIN (next_frame);
      if (!FRAME_CHAIN_VALID (address, next_frame))
	return 0;
      address = FRAME_CHAIN_COMBINE (address, next_frame);
    }
  if (address == 0)
    return 0;

  prev = (struct frame_info *)
    obstack_alloc (&frame_cache_obstack,
		   sizeof (struct frame_info));

  if (next_frame)
    next_frame->prev = prev;
  prev->next = next_frame;
  prev->prev = (struct frame_info *) 0;
  prev->frame = address;
  prev->next_frame = prev->next ? prev->next->frame : 0;

#ifdef INIT_EXTRA_FRAME_INFO
  INIT_EXTRA_FRAME_INFO(fromleaf, prev);
#endif

  /* This entry is in the frame queue now, which is good since
     FRAME_SAVED_PC may use that queue to figure out it's value
     (see tm-sparc.h).  We want the pc saved in the inferior frame. */
  INIT_FRAME_PC(fromleaf, prev);

  return prev;
}

CORE_ADDR
get_frame_pc (frame)
     FRAME frame;
{
  struct frame_info *fi;
  fi = get_frame_info (frame);
  return fi->pc;
}

#if defined (FRAME_FIND_SAVED_REGS)
/* Find the addresses in which registers are saved in FRAME.  */

void
get_frame_saved_regs (frame_info_addr, saved_regs_addr)
     struct frame_info *frame_info_addr;
     struct frame_saved_regs *saved_regs_addr;
{
  FRAME_FIND_SAVED_REGS (frame_info_addr, *saved_regs_addr);
}
#endif

/* Return the innermost lexical block in execution
   in a specified stack frame.  The frame address is assumed valid.  */

struct block *
get_frame_block (frame)
     FRAME frame;
{
  struct frame_info *fi;
  CORE_ADDR pc;

  fi = get_frame_info (frame);

  pc = fi->pc;
  if (fi->next_frame != 0)
    /* We are not in the innermost frame.  We need to subtract one to
       get the correct block, in case the call instruction was the
       last instruction of the block.  If there are any machines on
       which the saved pc does not point to after the call insn, we
       probably want to make fi->pc point after the call insn anyway.  */
    --pc;
  return block_for_pc (pc);
}

struct block *
get_current_block ()
{
  return block_for_pc (read_pc ());
}

CORE_ADDR
get_pc_function_start (pc)
     CORE_ADDR pc;
{
  register struct block *bl;
  register struct symbol *symbol;
  register struct minimal_symbol *msymbol;
  CORE_ADDR fstart;

  if ((bl = block_for_pc (pc)) != NULL &&
      (symbol = block_function (bl)) != NULL)
    {
      bl = SYMBOL_BLOCK_VALUE (symbol);
      fstart = BLOCK_START (bl);
    }
  else if ((msymbol = lookup_minimal_symbol_by_pc (pc)) != NULL)
    {
      fstart = msymbol -> address;
    }
  else
    {
      fstart = 0;
    }
  return (fstart);
}

/* Return the symbol for the function executing in frame FRAME.  */

struct symbol *
get_frame_function (frame)
     FRAME frame;
{
  register struct block *bl = get_frame_block (frame);
  if (bl == 0)
    return 0;
  return block_function (bl);
}

/* Return the blockvector immediately containing the innermost lexical block
   containing the specified pc value, or 0 if there is none.
   PINDEX is a pointer to the index value of the block.  If PINDEX
   is NULL, we don't pass this information back to the caller.  */

struct blockvector *
blockvector_for_pc (pc, pindex)
     register CORE_ADDR pc;
     int *pindex;
{
  register struct block *b;
  register int bot, top, half;
  register struct symtab *s;
  struct blockvector *bl;

  /* First search all symtabs for one whose file contains our pc */
  s = find_pc_symtab (pc);
  if (s == 0)
    return 0;

  bl = BLOCKVECTOR (s);
  b = BLOCKVECTOR_BLOCK (bl, 0);

  /* Then search that symtab for the smallest block that wins.  */
  /* Use binary search to find the last block that starts before PC.  */

  bot = 0;
  top = BLOCKVECTOR_NBLOCKS (bl);

  while (top - bot > 1)
    {
      half = (top - bot + 1) >> 1;
      b = BLOCKVECTOR_BLOCK (bl, bot + half);
      if (BLOCK_START (b) <= pc)
	bot += half;
      else
	top = bot + half;
    }

  /* Now search backward for a block that ends after PC.  */

  while (bot >= 0)
    {
      b = BLOCKVECTOR_BLOCK (bl, bot);
      if (BLOCK_END (b) > pc)
	{
	  if (pindex)
	    *pindex = bot;
	  return bl;
	}
      bot--;
    }

  return 0;
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  */

struct block *
block_for_pc (pc)
     register CORE_ADDR pc;
{
  register struct blockvector *bl;
  int index;

  bl = blockvector_for_pc (pc, &index);
  if (bl)
    return BLOCKVECTOR_BLOCK (bl, index);
  return 0;
}

/* Return the function containing pc value PC.
   Returns 0 if function is not known.  */

struct symbol *
find_pc_function (pc)
     CORE_ADDR pc;
{
  register struct block *b = block_for_pc (pc);
  if (b == 0)
    return 0;
  return block_function (b);
}

/* These variables are used to cache the most recent result
 * of find_pc_partial_function. */

static CORE_ADDR cache_pc_function_low = 0;
static CORE_ADDR cache_pc_function_high = 0;
static char *cache_pc_function_name = 0;

/* Clear cache, e.g. when symbol table is discarded. */

void
clear_pc_function_cache()
{
  cache_pc_function_low = 0;
  cache_pc_function_high = 0;
  cache_pc_function_name = (char *)0;
}

/* Finds the "function" (text symbol) that is smaller than PC
   but greatest of all of the potential text symbols.  Sets
   *NAME and/or *ADDRESS conditionally if that pointer is non-zero.
   Returns 0 if it couldn't find anything, 1 if it did.  On a zero
   return, *NAME and *ADDRESS are always set to zero.  On a 1 return,
   *NAME and *ADDRESS contain real information.  */

int
find_pc_partial_function (pc, name, address)
     CORE_ADDR pc;
     char **name;
     CORE_ADDR *address;
{
  struct partial_symtab *pst;
  struct symbol *f;
  struct minimal_symbol *msymbol;
  struct partial_symbol *psb;

  if (pc >= cache_pc_function_low && pc < cache_pc_function_high)
    {
	if (address)
	    *address = cache_pc_function_low;
	if (name)
	    *name = cache_pc_function_name;
	return 1;
    }

  pst = find_pc_psymtab (pc);
  if (pst)
    {
      if (pst->readin)
	{
	  /* The information we want has already been read in.
	     We can go to the already readin symbols and we'll get
	     the best possible answer.  */
	  f = find_pc_function (pc);
	  if (!f)
	    {
	    return_error:
	      /* No available symbol.  */
	      if (name != 0)
		*name = 0;
	      if (address != 0)
		*address = 0;
	      return 0;
	    }

	  cache_pc_function_low = BLOCK_START (SYMBOL_BLOCK_VALUE (f));
	  cache_pc_function_high = BLOCK_END (SYMBOL_BLOCK_VALUE (f));
	  cache_pc_function_name = SYMBOL_NAME (f);
	  if (name)
	    *name = cache_pc_function_name;
	  if (address)
	    *address = cache_pc_function_low;
	  return 1;
	}

      /* Get the information from a combination of the pst
	 (static symbols), and the minimal symbol table (extern
	 symbols).  */
      msymbol = lookup_minimal_symbol_by_pc (pc);
      psb = find_pc_psymbol (pst, pc);

      if (!psb && (msymbol == NULL))
	{
	  goto return_error;
	}
      if (psb
	  && (msymbol == NULL
	      || (SYMBOL_VALUE_ADDRESS (psb)
		  >= msymbol -> address)))
	{
	  /* This case isn't being cached currently. */
	  if (address)
	    *address = SYMBOL_VALUE_ADDRESS (psb);
	  if (name)
	    *name = SYMBOL_NAME (psb);
	  return 1;
	}
    }
  else
    /* Must be in the minimal symbol table.  */
    {
      msymbol = lookup_minimal_symbol_by_pc (pc);
      if (msymbol == NULL)
	goto return_error;
    }

  {
    if (msymbol -> type == mst_text)
      cache_pc_function_low = msymbol -> address;
    else
      /* It is a transfer table for Sun shared libraries.  */
      cache_pc_function_low = pc - FUNCTION_START_OFFSET;
  }
  cache_pc_function_name = msymbol -> name;
  /* FIXME:  Deal with bumping into end of minimal symbols for a given
     objfile, and what about testing for mst_text again? */
  if ((msymbol + 1) -> name != NULL)
    cache_pc_function_high = (msymbol + 1) -> address;
  else
    cache_pc_function_high = cache_pc_function_low + 1;
  if (address)
    *address = cache_pc_function_low;
  if (name)
    *name = cache_pc_function_name;
  return 1;
}

/* Return the innermost stack frame executing inside of the specified block,
   or zero if there is no such frame.  */

#if 0	/* Currently unused */

FRAME
block_innermost_frame (block)
     struct block *block;
{
  struct frame_info *fi;
  register FRAME frame;
  register CORE_ADDR start = BLOCK_START (block);
  register CORE_ADDR end = BLOCK_END (block);

  frame = 0;
  while (1)
    {
      frame = get_prev_frame (frame);
      if (frame == 0)
	return 0;
      fi = get_frame_info (frame);
      if (fi->pc >= start && fi->pc < end)
	return frame;
    }
}

#endif	/* 0 */

void
_initialize_blockframe ()
{
  obstack_init (&frame_cache_obstack);
}
