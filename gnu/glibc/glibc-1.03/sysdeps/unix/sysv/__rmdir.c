/* Copyright (C) 1992 Free Software Foundation, Inc.
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
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>


/* Create a directory named PATH with protections MODE.  */
int
DEFUN(__rmdir, (path), CONST char *path)
{
  char *cmd = __alloca (80 + strlen (path));
  char *p;
  int status;
  int save;
  struct stat statbuf;

  if (path == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  /* Check for some errors.  */
  if (__stat (path, &statbuf) < 0)
    return -1;
  if (!S_ISDIR (statbuf.st_mode))
    {
      errno = ENOTDIR;
      return -1;
    }

  p = cmd;
  *p++ = 'r';
  *p++ = 'm';
  *p++ = 'd';
  *p++ = 'i';
  *p++ = 'r';
  *p++ = ' ';

  strcpy (p, path);

  save = errno;
  /* If system doesn't set errno, but the rmdir fails, we really
     have no idea what went wrong.  EIO is the vaguest error I
     can think of, so I'll use that.  */
  errno = EIO;
  status = system (cmd);
  if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
    {
      return 0;
      errno = save;
    }
  else
    return -1;
}
