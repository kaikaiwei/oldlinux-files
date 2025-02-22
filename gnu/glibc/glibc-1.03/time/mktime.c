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
#include <limits.h>
#include <time.h>


/* Defined in offtime.c.  */
extern CONST unsigned short int __mon_lengths[2][12];


#define	invalid()	return (time_t) -1

/* Return the `time_t' representation of TP and normalizes TP.
   Return (time_t) -1 if TP is not representable as a `time_t'.
   Note that 31 Dec 1969 23:59:59 is not representable
   because it is represented as (time_t) -1.  */
time_t
DEFUN(mktime, (tp), register struct tm *tp)
{
  static struct tm min, max;
  static char init = 0;

  register time_t result;
  register time_t t;
  register int i;
  register CONST unsigned short *l;
  register struct tm *new;
  time_t end;

  if (tp == NULL)
    {
      errno = EINVAL;
      invalid();
    }

  if (!init)
    {
      init = 1;
      end = (time_t) LONG_MIN;
      new = gmtime(&end);
      if (new != NULL)
	min = *new;
      else
	min.tm_sec = min.tm_min = min.tm_hour =
	  min.tm_mday = min.tm_mon = min.tm_year = INT_MIN;

      end = (time_t) LONG_MAX;
      new = gmtime(&end);
      if (new != NULL)
	max = *new;
      else
	max.tm_sec = max.tm_min = max.tm_hour =
	  max.tm_mday = max.tm_mon = max.tm_year = INT_MAX;
    }

  /* Make all the elements of TP that we pay attention to
     be within the ranges of reasonable values for those things.  */
#define	normalize(elt, min, max, nextelt) \
  while (tp->elt < min)							      \
    {									      \
      --tp->nextelt;							      \
      tp->elt += max + 1;						      \
    }									      \
  while (tp->elt > max)							      \
    {									      \
      ++tp->nextelt;							      \
      tp->elt -= max + 1;						      \
    }

  normalize (tm_sec, 0, 59, tm_min);
  normalize (tm_min, 0, 59, tm_hour);
  normalize (tm_hour, 0, 24, tm_mday);

  /* Normalize the month first so we can use
     it to figure the range for the day.  */
  normalize (tm_mon, 0, 11, tm_year);
  normalize (tm_mday, 1, __mon_lengths[__isleap (tp->tm_year)][tp->tm_mon],
	     tm_mon);

  /* Normalize the month again, since normalizing
     the day may have pushed it out of range.  */
  normalize (tm_mon, 0, 11, tm_year);

  /* Normalize the day again, because normalizing
     the month may have changed the range.  */
  normalize (tm_mday, 1, __mon_lengths[__isleap (tp->tm_year)][tp->tm_mon],
	     tm_mon);

  /* Check for out-of-range values.  */
#define	lowhigh(field, minmax, cmp)	(tp->field cmp minmax.field)
#define	low(field)			lowhigh(field, min, <)
#define	high(field)			lowhigh(field, max, >)
#define	oor(field)			(low(field) || high(field))
#define	lowbound(field)			(tp->field == min.field)
#define	highbound(field)		(tp->field == max.field)
  if (oor(tm_year))
    invalid();
  else if (lowbound(tm_year))
    {
      if (low(tm_mon))
	invalid();
      else if (lowbound(tm_mon))
	{
	  if (low(tm_mday))
	    invalid();
	  else if (lowbound(tm_mday))
	    {
	      if (low(tm_hour))
		invalid();
	      else if (lowbound(tm_hour))
		{
		  if (low(tm_min))
		    invalid();
		  else if (lowbound(tm_min))
		    {
		      if (low(tm_sec))
			invalid();
		    }
		}
	    }
	}
    }
  else if (highbound(tm_year))
    {
      if (high(tm_mon))
	invalid();
      else if (highbound(tm_mon))
	{
	  if (high(tm_mday))
	    invalid();
	  else if (highbound(tm_mday))
	    {
	      if (high(tm_hour))
		invalid();
	      else if (highbound(tm_hour))
		{
		  if (high(tm_min))
		    invalid();
		  else if (highbound(tm_min))
		    {
		      if (high(tm_sec))
			invalid();
		    }
		}
	    }
	}
    }

  t = 0;
  for (i = 1970; i > 1900 + tp->tm_year; --i)
    t -= __isleap(i) ? 366 : 365;
  for (i = 1970; i < 1900 + tp->tm_year; ++i)
    t += __isleap(i) ? 366 : 365;
  l = __mon_lengths[__isleap(1900 + tp->tm_year)];
  for (i = 0; i < tp->tm_mon; ++i)
    t += l[i];
  t += tp->tm_mday - 1;
  result = ((t * 60 * 60 * 24) +
	    (tp->tm_hour * 60 * 60) +
	    (tp->tm_min * 60) +
	    tp->tm_sec);

  end = result;
  if (tp->tm_isdst < 0)
    new = localtime(&end);
  else
    new = gmtime(&end);
  if (new == NULL)
    invalid();
  new->tm_isdst = tp->tm_isdst;
  *tp = *new;

  return result;
}
