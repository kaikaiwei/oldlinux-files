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
#include <ctype.h>
#include <localeinfo.h>

/* Defined in locale/locale-C-ctype.c.  */
extern CONST unsigned short int __ctype_b_C[];
extern CONST unsigned char __ctype_tolower_C[];
extern CONST unsigned char __ctype_toupper_C[];

CONST unsigned short int *__ctype_b = __ctype_b_C + 1;
CONST unsigned char *__ctype_tolower = __ctype_tolower_C + 1;
CONST unsigned char *__ctype_toupper = __ctype_toupper_C + 1;
