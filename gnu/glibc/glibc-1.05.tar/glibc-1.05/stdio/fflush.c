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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Flush STREAM's buffer.
   If STREAM is NULL, flush the buffers of all streams that are writing.  */
int
DEFUN(fflush, (stream), register FILE *stream)
{
  if (stream == NULL)
    {
      int lossage = 0;
      for (stream = __stdio_head; stream != NULL; stream = stream->__next)
	if (__validfp(stream) && stream->__mode.__write)
	  lossage |= fflush(stream) == EOF;
      return lossage ? EOF : 0;
    }

  if (!__validfp(stream) || !stream->__mode.__write)
    {
      errno = EINVAL;
      return EOF;
    }

  return __flshfp(stream, EOF);
}
