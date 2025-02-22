/* Definitions for symbol-reading containing "stabs", for GDB.
   Copyright 1992 Free Software Foundation, Inc.
   Contributed by Cygnus Support.  Written by John Gilmore.

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

/* This file exists to hold the common definitions required of all the
   symbol-readers that end up using stabs.  The common use of these 
   `symbol-type-specific' customizations of the generic data structures
   makes the stabs-oriented symbol readers able to call each others'
   functions as required.  */

#if !defined (GDBSTABS_H)
#define GDBSTABS_H

/* Offsets in the psymtab's section_offsets array for various kinds of
   stabs symbols.  Every psymtab built from stabs will have these offsets
   filled in by these guidelines, so that when actually reading symbols, the
   proper offset can simply be selected and added to the symbol value.  */

#define	SECT_OFF_TEXT	0
#define	SECT_OFF_DATA	1
#define	SECT_OFF_BSS	2
#define	SECT_OFF_RODATA	3
#define	SECT_OFF_MAX	4		/* Count of possible values */

/* The stab_section_info chain remembers info from the ELF symbol table,
   while psymtabs are being built for the other symbol tables in the 
   objfile.  It is destroyed at the complation of psymtab-reading.
   Any info that was used from it has been copied into psymtabs.  */

struct stab_section_info {
  char *filename;
  CORE_ADDR sections[SECT_OFF_MAX];
  struct stab_section_info *next;
  int found;		/* Count of times it's found in searching */
};

/* Information is passed among various dbxread routines for accessing
   symbol files.  A pointer to this structure is kept in the sym_private
   field of the objfile struct.  */
 
struct dbx_symfile_info {
  asection *text_sect;		/* Text section accessor */
  int symcount;			/* How many symbols are there in the file */
  char *stringtab;		/* The actual string table */
  int stringtab_size;		/* Its size */
  off_t symtab_offset;		/* Offset in file to symbol table */
  int symbol_size;		/* Bytes in a single symbol */
  struct stab_section_info *stab_section_info; 	/* section starting points
				   of the original .o files before linking. */
/* FIXME:  HP kludges that shouldn't be here, probably.  */
  int hp_symcount;
  char *hp_stringtab;
  int hp_stringtab_size;
  off_t hp_symtab_offset;
};

#define DBX_SYMFILE_INFO(o)	((struct dbx_symfile_info *)((o)->sym_private))
#define DBX_TEXT_SECT(o)	(DBX_SYMFILE_INFO(o)->text_sect)
#define DBX_SYMCOUNT(o)		(DBX_SYMFILE_INFO(o)->symcount)
#define DBX_STRINGTAB(o)	(DBX_SYMFILE_INFO(o)->stringtab)
#define DBX_STRINGTAB_SIZE(o)	(DBX_SYMFILE_INFO(o)->stringtab_size)
#define DBX_SYMTAB_OFFSET(o)	(DBX_SYMFILE_INFO(o)->symtab_offset)
#define DBX_SYMBOL_SIZE(o)	(DBX_SYMFILE_INFO(o)->symbol_size)
/* We don't use a macro for stab_section_info.  */
#define HP_SYMCOUNT(o)		(DBX_SYMFILE_INFO(o)->hp_symcount)
#define HP_STRINGTAB(o)		(DBX_SYMFILE_INFO(o)->hp_stringtab)
#define HP_STRINGTAB_SIZE(o)	(DBX_SYMFILE_INFO(o)->hp_stringtab_size)
#define HP_SYMTAB_OFFSET(o)	(DBX_SYMFILE_INFO(o)->hp_symtab_offset)

#endif /* GDBSTABS_H */
