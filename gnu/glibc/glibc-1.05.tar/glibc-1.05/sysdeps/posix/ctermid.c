/* Copyright (C) 1991 Free Software Foundation, Inc.
This file is part of the GNU C Library.

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
#include <stdio.h>
#include <string.h>


/* Return the name of the controlling terminal.
   If S is not NULL, the name is copied into it (it should be at
   least L_ctermid bytes long), otherwise a static buffer is used.  */
char *
DEFUN(ctermid, (s), char *s)
{
  static char name[L_ctermid];

  if (name[0] == '\0')
    (void) strcpy(name, "/dev/tty");

  if (s == NULL)
    return(name);

  return(strcpy(s, name));
}
