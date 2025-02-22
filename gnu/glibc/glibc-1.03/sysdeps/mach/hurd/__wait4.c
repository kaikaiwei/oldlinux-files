/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <hurd.h>

pid_t
DEFUN(__wait4, (pid, stat_loc, options, usage),
      pid_t pid AND union wait *stat_loc AND int options AND PTR usage)
{
  pid_t dead;
  error_t err;

  err = _HURD_PORT_USE (&_hurd_proc, __proc_wait (port, pid, stat_loc,
						  options, usage, &dead));
  if (err)
    return __hurd_fail (err);
  return dead;
}
