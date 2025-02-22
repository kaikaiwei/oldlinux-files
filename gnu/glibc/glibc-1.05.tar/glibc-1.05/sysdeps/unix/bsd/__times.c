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
#include <errno.h>
#include <stddef.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>


/* Time the program started.  */
extern time_t _posix_start_time;

static clock_t
#ifdef	__GNUC__
__inline
#endif
DEFUN(timeval_to_clock_t, (tv), CONST struct timeval *tv)
{
  return (clock_t) ((tv->tv_sec * CLK_TCK) +
		    (tv->tv_usec * CLK_TCK / 1000000L));
}

/* Store the CPU time used by this process and all its
   dead children (and their dead children) in BUFFER.
   Return the elapsed real time, or (clock_t) -1 for errors.
   All times are in CLK_TCKths of a second.  */
clock_t
DEFUN(__times, (buffer), struct tms *buffer)
{
  struct rusage usage;

  if (buffer == NULL)
    {
      errno = EINVAL;
      return (clock_t) -1;
    }

  if (__getrusage(RUSAGE_SELF, &usage) < 0)
    return (clock_t) -1;
  buffer->tms_utime = (clock_t) timeval_to_clock_t(&usage.ru_utime);
  buffer->tms_stime = (clock_t) timeval_to_clock_t(&usage.ru_stime);

  if (__getrusage(RUSAGE_CHILDREN, &usage) < 0)
    return (clock_t) -1;
  buffer->tms_cutime = (clock_t) timeval_to_clock_t(&usage.ru_utime);
  buffer->tms_cstime = (clock_t) timeval_to_clock_t(&usage.ru_stime);

  return (time((time_t *) NULL) - _posix_start_time) * CLK_TCK;
}
