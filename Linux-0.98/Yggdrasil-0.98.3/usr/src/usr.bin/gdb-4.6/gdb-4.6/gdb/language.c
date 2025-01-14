/* Multiple source language support for GDB.
   Copyright 1991, 1992 Free Software Foundation, Inc.
   Contributed by the Department of Computer Science at the State University
   of New York at Buffalo.

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

/* This file contains functions that return things that are specific
   to languages.  Each function should examine current_language if necessary,
   and return the appropriate result. */

/* FIXME:  Most of these would be better organized as macros which
   return data out of a "language-specific" struct pointer that is set
   whenever the working language changes.  That would be a lot faster.  */

#include "defs.h"
#include <string.h>
#include <varargs.h>

#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcmd.h"
#include "frame.h"
#include "expression.h"
#include "language.h"
#include "target.h"
#include "parser-defs.h"

static void
show_language_command PARAMS ((char *, int));

static void
set_language_command PARAMS ((char *, int));

static void
show_type_command PARAMS ((char *, int));

static void
set_type_command PARAMS ((char *, int));

static void
show_range_command PARAMS ((char *, int));

static void
set_range_command PARAMS ((char *, int));

static void
set_range_str PARAMS ((void));

static void
set_type_str PARAMS ((void));

static void
set_lang_str PARAMS ((void));

static void
unk_lang_error PARAMS ((char *));

static int
unk_lang_parser PARAMS ((void));

static void
show_check PARAMS ((char *, int));

static void
set_check PARAMS ((char *, int));

static void
set_type_range PARAMS ((void));

/* Forward declaration */
extern const struct language_defn unknown_language_defn;
extern char *warning_pre_print;
  
/* The current (default at startup) state of type and range checking.
    (If the modes are set to "auto", though, these are changed based
    on the default language at startup, and then again based on the
    language of the first source file.  */

enum range_mode range_mode = range_mode_auto;
enum range_check range_check = range_check_off;
enum type_mode type_mode = type_mode_auto;
enum type_check type_check = type_check_off;

/* The current language and language_mode (see language.h) */

const struct language_defn *current_language = &unknown_language_defn;
enum language_mode language_mode = language_mode_auto;

/* The list of supported languages.  The list itself is malloc'd.  */

static const struct language_defn **languages;
static unsigned languages_size;
static unsigned languages_allocsize;
#define	DEFAULT_ALLOCSIZE 3

/* The "set language/type/range" commands all put stuff in these
   buffers.  This is to make them work as set/show commands.  The
   user's string is copied here, then the set_* commands look at
   them and update them to something that looks nice when it is
   printed out. */

static char *language;
static char *type;
static char *range;

/* Warning issued when current_language and the language of the current
   frame do not match. */
char lang_frame_mismatch_warn[] =
	"Warning: the current language does not match this frame.";


/* This page contains the functions corresponding to GDB commands
   and their helpers. */

/* Show command.  Display a warning if the language set
   does not match the frame. */
static void
show_language_command (ignore, from_tty)
   char *ignore;
   int from_tty;
{
   enum language flang;		/* The language of the current frame */

   flang = get_frame_language();
   if (flang != language_unknown &&
      language_mode == language_mode_manual &&
      current_language->la_language != flang)
      printf_filtered("%s\n",lang_frame_mismatch_warn);
}

/* Set command.  Change the current working language. */
static void
set_language_command (ignore, from_tty)
   char *ignore;
   int from_tty;
{
  int i;
  enum language flang;
  char *err_lang;

  /* FIXME -- do this from the list, with HELP.  */
  if (!language || !language[0]) {
    printf("The currently understood settings are:\n\n\
local or auto    Automatic setting based on source file\n\
c                Use the C language\n\
c++              Use the C++ language\n\
modula-2         Use the Modula-2 language\n");
    /* Restore the silly string. */
    set_language(current_language->la_language);
    return;
  }

  /* Search the list of languages for a match.  */
  for (i = 0; i < languages_size; i++) {
    if (!strcmp (languages[i]->la_name, language)) {
      /* Found it!  Go into manual mode, and use this language.  */
      if (languages[i]->la_language == language_auto) {
	/* Enter auto mode.  Set to the current frame's language, if known.  */
	language_mode = language_mode_auto;
  	flang = get_frame_language();
	if (flang!=language_unknown)
	  set_language(flang);
	return;
      } else {
	/* Enter manual mode.  Set the specified language.  */
	language_mode = language_mode_manual;
	current_language = languages[i];
	set_type_range ();
	set_lang_str();
	return;
      }
    }
  }

  /* Reset the language (esp. the global string "language") to the 
     correct values. */
  err_lang=savestring(language,strlen(language));
  make_cleanup (free, err_lang);	/* Free it after error */
  set_language(current_language->la_language);
  error ("Unknown language `%s'.",err_lang);
}

/* Show command.  Display a warning if the type setting does
   not match the current language. */
static void
show_type_command(ignore, from_tty)
   char *ignore;
   int from_tty;
{
   if (type_check != current_language->la_type_check)
      printf(
"Warning: the current type check setting does not match the language.\n");
}

/* Set command.  Change the setting for type checking. */
static void
set_type_command(ignore, from_tty)
   char *ignore;
   int from_tty;
{
   if (!strcmp(type,"on"))
   {
      type_check = type_check_on;
      type_mode = type_mode_manual;
   }
   else if (!strcmp(type,"warn"))
   {
      type_check = type_check_warn;
      type_mode = type_mode_manual;
   }
   else if (!strcmp(type,"off"))
   {
      type_check = type_check_off;
      type_mode = type_mode_manual;
   }
   else if (!strcmp(type,"auto"))
   {
      type_mode = type_mode_auto;
      set_type_range();
      /* Avoid hitting the set_type_str call below.  We
         did it in set_type_range. */
      return;
   }
   set_type_str();
   show_type_command((char *)NULL, from_tty);
}

/* Show command.  Display a warning if the range setting does
   not match the current language. */
static void
show_range_command(ignore, from_tty)
   char *ignore;
   int from_tty;
{

   if (range_check != current_language->la_range_check)
      printf(
"Warning: the current range check setting does not match the language.\n");
}

/* Set command.  Change the setting for range checking. */
static void
set_range_command(ignore, from_tty)
   char *ignore;
   int from_tty;
{
   if (!strcmp(range,"on"))
   {
      range_check = range_check_on;
      range_mode = range_mode_manual;
   }
   else if (!strcmp(range,"warn"))
   {
      range_check = range_check_warn;
      range_mode = range_mode_manual;
   }
   else if (!strcmp(range,"off"))
   {
      range_check = range_check_off;
      range_mode = range_mode_manual;
   }
   else if (!strcmp(range,"auto"))
   {
      range_mode = range_mode_auto;
      set_type_range();
      /* Avoid hitting the set_range_str call below.  We
	 did it in set_type_range. */
      return;
   }
   set_range_str();
   show_range_command((char *)0, from_tty);
}

/* Set the status of range and type checking based on
   the current modes and the current language.
   If SHOW is non-zero, then print out the current language,
   type and range checking status. */
static void
set_type_range()
{

  if (range_mode == range_mode_auto)
    range_check = current_language->la_range_check;

  if (type_mode == type_mode_auto)
    type_check = current_language->la_type_check;

  set_type_str();
  set_range_str();
}

/* Set current language to (enum language) LANG.  */

void
set_language(lang)
   enum language lang;
{
  int i;

  for (i = 0; i < languages_size; i++) {
    if (languages[i]->la_language == lang) {
      current_language = languages[i];
      set_type_range ();
      set_lang_str();
      break;
    }
  }
}

/* This page contains functions that update the global vars
   language, type and range. */
static void
set_lang_str()
{
   char *prefix = "";

   free (language);
   if (language_mode == language_mode_auto)
      prefix = "auto; currently ";

   language = concat(prefix, current_language->la_name, NULL);
}

static void
set_type_str()
{
   char *tmp, *prefix = "";

   free (type);
   if (type_mode==type_mode_auto)
      prefix = "auto; currently ";

   switch(type_check)
   {
   case type_check_on:
      tmp = "on";
      break;
   case type_check_off:
      tmp = "off";
      break;
   case type_check_warn:
      tmp = "warn";
      break;
      default:
      error ("Unrecognized type check setting.");
   }

   type = concat(prefix,tmp,NULL);
}

static void
set_range_str()
{
   char *tmp, *pref = "";

   free (range);
   if (range_mode==range_mode_auto)
      pref = "auto; currently ";

   switch(range_check)
   {
   case range_check_on:
      tmp = "on";
      break;
   case range_check_off:
      tmp = "off";
      break;
   case range_check_warn:
      tmp = "warn";
      break;
      default:
      error ("Unrecognized range check setting.");
   }

   range = concat(pref,tmp,NULL);
}


/* Print out the current language settings: language, range and
   type checking.  If QUIETLY, print only what has changed.  */
void
language_info (quietly)
     int quietly;
{
  /* FIXME:  quietly is ignored at the moment.  */
   printf("Current Language:  %s\n",language);
   show_language_command((char *)0, 1);
   printf("Type checking:     %s\n",type);
   show_type_command((char *)0, 1);
   printf("Range checking:    %s\n",range);
   show_range_command((char *)0, 1);
}

/* Return the result of a binary operation. */

#if 0	/* Currently unused */

struct type *
binop_result_type(v1,v2)
   value v1,v2;
{
   int l1,l2,size,uns;

   l1 = TYPE_LENGTH(VALUE_TYPE(v1));
   l2 = TYPE_LENGTH(VALUE_TYPE(v2));

   switch(current_language->la_language)
   {
   case language_c:
   case language_cplus:
      if (TYPE_CODE(VALUE_TYPE(v1))==TYPE_CODE_FLT)
	 return TYPE_CODE(VALUE_TYPE(v2)) == TYPE_CODE_FLT && l2 > l1 ?
	    VALUE_TYPE(v2) : VALUE_TYPE(v1);
      else if (TYPE_CODE(VALUE_TYPE(v2))==TYPE_CODE_FLT)
	 return TYPE_CODE(VALUE_TYPE(v1)) == TYPE_CODE_FLT && l1 > l2 ?
	    VALUE_TYPE(v1) : VALUE_TYPE(v2);
      else if (TYPE_UNSIGNED(VALUE_TYPE(v1)) && l1 > l2)
	 return VALUE_TYPE(v1);
      else if (TYPE_UNSIGNED(VALUE_TYPE(v2)) && l2 > l1)
	 return VALUE_TYPE(v2);
      else  /* Both are signed.  Result is the longer type */
	 return l1 > l2 ? VALUE_TYPE(v1) : VALUE_TYPE(v2);
      break;
   case language_m2:
      /* If we are doing type-checking, l1 should equal l2, so this is
	 not needed. */
      return l1 > l2 ? VALUE_TYPE(v1) : VALUE_TYPE(v2);
      break;
   }
   abort();
   return (struct type *)0;	/* For lint */
}

#endif	/* 0 */


/* This page contains functions that return format strings for
   printf for printing out numbers in different formats */

/* Returns the appropriate printf format for hexadecimal
   numbers. */
char *
local_hex_format_custom(pre)
   char *pre;
{
   static char form[50];

   strcpy (form, current_language->la_hex_format_pre);
   strcat (form, pre);
   strcat (form, current_language->la_hex_format_suf);
   return form;
}

/* Converts a number to hexadecimal and stores it in a static
   string.  Returns a pointer to this string. */
char *
local_hex_string (num)
   int num;
{
   static char res[50];

   sprintf (res, current_language->la_hex_format, num);
   return res;
}

/* Converts a number to custom hexadecimal and stores it in a static
   string.  Returns a pointer to this string. */
char *
local_hex_string_custom(num,pre)
   int num;
   char *pre;
{
   static char res[50];

   sprintf (res, local_hex_format_custom(pre), num);
   return res;
}

/* Returns the appropriate printf format for octal
   numbers. */
char *
local_octal_format_custom(pre)
   char *pre;
{
   static char form[50];

   strcpy (form, current_language->la_octal_format_pre);
   strcat (form, pre);
   strcat (form, current_language->la_octal_format_suf);
   return form;
}

/* This page contains functions that are used in type/range checking.
   They all return zero if the type/range check fails.

   It is hoped that these will make extending GDB to parse different
   languages a little easier.  These are primarily used in eval.c when
   evaluating expressions and making sure that their types are correct.
   Instead of having a mess of conjucted/disjuncted expressions in an "if",
   the ideas of type can be wrapped up in the following functions.

   Note that some of them are not currently dependent upon which language
   is currently being parsed.  For example, floats are the same in
   C and Modula-2 (ie. the only floating point type has TYPE_CODE of
   TYPE_CODE_FLT), while booleans are different. */

/* Returns non-zero if its argument is a simple type.  This is the same for
   both Modula-2 and for C.  In the C case, TYPE_CODE_CHAR will never occur,
   and thus will never cause the failure of the test. */
int
simple_type(type)
    struct type *type;
{
  switch (TYPE_CODE (type)) {
  case TYPE_CODE_INT:
  case TYPE_CODE_CHAR:
  case TYPE_CODE_ENUM:
  case TYPE_CODE_FLT:
  case TYPE_CODE_RANGE:
  case TYPE_CODE_BOOL:
    return 1;

  default:
    return 0;
  }
}

/* Returns non-zero if its argument is of an ordered type. */
int
ordered_type (type)
   struct type *type;
{
  switch (TYPE_CODE (type)) {
  case TYPE_CODE_INT:
  case TYPE_CODE_CHAR:
  case TYPE_CODE_ENUM:
  case TYPE_CODE_FLT:
  case TYPE_CODE_RANGE:
    return 1;

  default:
    return 0;
  }
}

/* Returns non-zero if the two types are the same */
int
same_type (arg1, arg2)
   struct type *arg1, *arg2;
{
   if (structured_type(arg1) ? !structured_type(arg2) : structured_type(arg2))
      /* One is structured and one isn't */
      return 0;
   else if (structured_type(arg1) && structured_type(arg2))
      return arg1 == arg2;
   else if (numeric_type(arg1) && numeric_type(arg2))
      return (TYPE_CODE(arg2) == TYPE_CODE(arg1)) &&
	 (TYPE_UNSIGNED(arg1) == TYPE_UNSIGNED(arg2))
	    ? 1 : 0;
   else
      return arg1==arg2;
}

/* Returns non-zero if the type is integral */
int
integral_type (type)
   struct type *type;
{
   switch(current_language->la_language)
   {
   case language_c:
   case language_cplus:
      return (TYPE_CODE(type) != TYPE_CODE_INT) &&
	 (TYPE_CODE(type) != TYPE_CODE_ENUM) ? 0 : 1;
   case language_m2:
      return TYPE_CODE(type) != TYPE_CODE_INT ? 0 : 1;
   default:
      error ("Language not supported.");
   }
}

/* Returns non-zero if the value is numeric */
int
numeric_type (type)
   struct type *type;
{
  switch (TYPE_CODE (type)) {
  case TYPE_CODE_INT:
  case TYPE_CODE_FLT:
    return 1;

  default:
    return 0;
  }
}

/* Returns non-zero if the value is a character type */
int
character_type (type)
   struct type *type;
{
   switch(current_language->la_language)
   {
   case language_m2:
      return TYPE_CODE(type) != TYPE_CODE_CHAR ? 0 : 1;

   case language_c:
   case language_cplus:
      return (TYPE_CODE(type) == TYPE_CODE_INT) &&
	 TYPE_LENGTH(type) == sizeof(char)
	 ? 1 : 0;
   default:
      return (0);
   }
}

/* Returns non-zero if the value is a boolean type */
int
boolean_type (type)
   struct type *type;
{
   switch(current_language->la_language)
   {
   case language_m2:
      return TYPE_CODE(type) != TYPE_CODE_BOOL ? 0 : 1;

   case language_c:
   case language_cplus:
      return TYPE_CODE(type) != TYPE_CODE_INT ? 0 : 1;
   default:
      return (0);
   }
}

/* Returns non-zero if the value is a floating-point type */
int
float_type (type)
   struct type *type;
{
   return TYPE_CODE(type) == TYPE_CODE_FLT;
}

/* Returns non-zero if the value is a pointer type */
int
pointer_type(type)
   struct type *type;
{
   return TYPE_CODE(type) == TYPE_CODE_PTR ||
      TYPE_CODE(type) == TYPE_CODE_REF;
}

/* Returns non-zero if the value is a structured type */
int
structured_type(type)
   struct type *type;
{
   switch(current_language->la_language)
   {
   case language_c:
   case language_cplus:
      return (TYPE_CODE(type) == TYPE_CODE_STRUCT) ||
	 (TYPE_CODE(type) == TYPE_CODE_UNION) ||
	    (TYPE_CODE(type) == TYPE_CODE_ARRAY);
   case language_m2:
      return (TYPE_CODE(type) == TYPE_CODE_STRUCT) ||
	 (TYPE_CODE(type) == TYPE_CODE_SET) ||
	    (TYPE_CODE(type) == TYPE_CODE_ARRAY);
   default:
      return (0);
   }
}

/* This page contains functions that return info about
   (struct value) values used in GDB. */

/* Returns non-zero if the value VAL represents a true value. */
int
value_true(val)
     value val;
{
  int len, i;
  struct type *type;
  LONGEST v;

  switch (current_language->la_language) {

  case language_c:
  case language_cplus:
    return !value_zerop (val);

  case language_m2:
    type = VALUE_TYPE(val);
    if (TYPE_CODE (type) != TYPE_CODE_BOOL)
      return 0;		/* Not a BOOLEAN at all */
    /* Search the fields for one that matches the current value. */
    len = TYPE_NFIELDS (type);
    v = value_as_long (val);
    for (i = 0; i < len; i++)
      {
	QUIT;
	if (v == TYPE_FIELD_BITPOS (type, i))
	  break;
      }
    if (i >= len)
      return 0;		/* Not a valid BOOLEAN value */
    if (!strcmp ("TRUE", TYPE_FIELD_NAME(VALUE_TYPE(val), i)))
      return 1;		/* BOOLEAN with value TRUE */
    else
      return 0;		/* BOOLEAN with value FALSE */
    break;

  default:
    error ("Language not supported.");
  }
}

/* Returns non-zero if the operator OP is defined on
   the values ARG1 and ARG2. */

#if 0	/* Currently unused */

void
binop_type_check(arg1,arg2,op)
   value arg1,arg2;
   int op;
{
   struct type *t1, *t2;

   /* If we're not checking types, always return success. */
   if (!STRICT_TYPE)
      return;

   t1=VALUE_TYPE(arg1);
   if (arg2!=(value)NULL)
      t2=VALUE_TYPE(arg2);
   else
      t2=NULL;

   switch(op)
   {
   case BINOP_ADD:
   case BINOP_SUB:
      if ((numeric_type(t1) && pointer_type(t2)) ||
	 (pointer_type(t1) && numeric_type(t2)))
      {
	 warning ("combining pointer and integer.\n");
	 break;
      }
   case BINOP_MUL:
   case BINOP_LSH:
   case BINOP_RSH:
      if (!numeric_type(t1) || !numeric_type(t2))
	 type_op_error ("Arguments to %s must be numbers.",op);
      else if (!same_type(t1,t2))
	 type_op_error ("Arguments to %s must be of the same type.",op);
      break;

   case BINOP_AND:
   case BINOP_OR:
      if (!boolean_type(t1) || !boolean_type(t2))
	 type_op_error ("Arguments to %s must be of boolean type.",op);
      break;

   case BINOP_EQUAL:
      if ((pointer_type(t1) && !(pointer_type(t2) || integral_type(t2))) ||
	 (pointer_type(t2) && !(pointer_type(t1) || integral_type(t1))))
	 type_op_error ("A pointer can only be compared to an integer or pointer.",op);
      else if ((pointer_type(t1) && integral_type(t2)) ||
	 (integral_type(t1) && pointer_type(t2)))
      {
	 warning ("combining integer and pointer.\n");
	 break;
      }
      else if (!simple_type(t1) || !simple_type(t2))
	 type_op_error ("Arguments to %s must be of simple type.",op);
      else if (!same_type(t1,t2))
	 type_op_error ("Arguments to %s must be of the same type.",op);
      break;

   case BINOP_REM:
      if (!integral_type(t1) || !integral_type(t2))
	 type_op_error ("Arguments to %s must be of integral type.",op);
      break;

   case BINOP_LESS:
   case BINOP_GTR:
   case BINOP_LEQ:
   case BINOP_GEQ:
      if (!ordered_type(t1) || !ordered_type(t2))
	 type_op_error ("Arguments to %s must be of ordered type.",op);
      else if (!same_type(t1,t2))
	 type_op_error ("Arguments to %s must be of the same type.",op);
      break;

   case BINOP_ASSIGN:
      if (pointer_type(t1) && !integral_type(t2))
	 type_op_error ("A pointer can only be assigned an integer.",op);
      else if (pointer_type(t1) && integral_type(t2))
      {
	 warning ("combining integer and pointer.");
	 break;
      }
      else if (!simple_type(t1) || !simple_type(t2))
	 type_op_error ("Arguments to %s must be of simple type.",op);
      else if (!same_type(t1,t2))
	 type_op_error ("Arguments to %s must be of the same type.",op);
      break;

   /* Unary checks -- arg2 is null */

   case UNOP_ZEROP:
      if (!boolean_type(t1))
	 type_op_error ("Argument to %s must be of boolean type.",op);
      break;

   case UNOP_PLUS:
   case UNOP_NEG:
      if (!numeric_type(t1))
	 type_op_error ("Argument to %s must be of numeric type.",op);
      break;

   case UNOP_IND:
      if (integral_type(t1))
      {
	 warning ("combining pointer and integer.\n");
	 break;
      }
      else if (!pointer_type(t1))
	 type_op_error ("Argument to %s must be a pointer.",op);
      break;

   case UNOP_PREINCREMENT:
   case UNOP_POSTINCREMENT:
   case UNOP_PREDECREMENT:
   case UNOP_POSTDECREMENT:
      if (!ordered_type(t1))
	 type_op_error ("Argument to %s must be of an ordered type.",op);
      break;

   default:
      /* Ok.  The following operators have different meanings in
	 different languages. */
      switch(current_language->la_language)
      {
#ifdef _LANG_c
      case language_c:
      case language_cplus:
	 switch(op)
	 {
	 case BINOP_DIV:
	    if (!numeric_type(t1) || !numeric_type(t2))
	       type_op_error ("Arguments to %s must be numbers.",op);
	    break;
	 }
	 break;
#endif

#ifdef _LANG_m2
      case language_m2:
	 switch(op)
	 {
	 case BINOP_DIV:
	    if (!float_type(t1) || !float_type(t2))
	       type_op_error ("Arguments to %s must be floating point numbers.",op);
	    break;
	 case BINOP_INTDIV:
	    if (!integral_type(t1) || !integral_type(t2))
	       type_op_error ("Arguments to %s must be of integral type.",op);
	    break;
	 }
#endif
      }
   }
}

#endif	/* 0 */


/* This page contains functions for the printing out of
   error messages that occur during type- and range-
   checking. */

/* Prints the format string FMT with the operator as a string
   corresponding to the opcode OP.  If FATAL is non-zero, then
   this is an error and error () is called.  Otherwise, it is
   a warning and printf() is called. */
void
op_error (fmt,op,fatal)
   char *fmt;
   enum exp_opcode op;
   int fatal;
{
   if (fatal)
      error (fmt,op_string(op));
   else
   {
      warning (fmt,op_string(op));
   }
}

/* These are called when a language fails a type- or range-check.
   The first argument should be a printf()-style format string, and
   the rest of the arguments should be its arguments.  If
   [type|range]_check is [type|range]_check_on, then return_to_top_level()
   is called in the style of error ().  Otherwise, the message is prefixed
   by the value of warning_pre_print and we do not return to the top level. */

void
type_error (va_alist)
     va_dcl
{
   va_list args;
   char *string;

   if (type_check==type_check_warn)
      fprintf(stderr,warning_pre_print);
   else
      target_terminal_ours();

   va_start (args);
   string = va_arg (args, char *);
   vfprintf (stderr, string, args);
   fprintf (stderr, "\n");
   va_end (args);
   if (type_check==type_check_on)
      return_to_top_level();
}

void
range_error (va_alist)
     va_dcl
{
   va_list args;
   char *string;

   if (range_check==range_check_warn)
      fprintf(stderr,warning_pre_print);
   else
      target_terminal_ours();

   va_start (args);
   string = va_arg (args, char *);
   vfprintf (stderr, string, args);
   fprintf (stderr, "\n");
   va_end (args);
   if (range_check==range_check_on)
      return_to_top_level();
}


/* This page contains miscellaneous functions */

/* Return the language as a string */
char *
language_str(lang)
   enum language lang;
{
  int i;

  for (i = 0; i < languages_size; i++) {
    if (languages[i]->la_language == lang) {
      return languages[i]->la_name;
    }
  }
  return "Unknown";
}

static void
set_check (ignore, from_tty)
   char *ignore;
   int from_tty;
{
   printf(
"\"set check\" must be followed by the name of a check subcommand.\n");
   help_list(setchecklist, "set check ", -1, stdout);
}

static void
show_check (ignore, from_tty)
   char *ignore;
   int from_tty;
{
   cmd_show_list(showchecklist, from_tty, "");
}

/* Add a language to the set of known languages.  */

void
add_language (lang)
     const struct language_defn *lang;
{
  if (lang->la_magic != LANG_MAGIC)
    {
      fprintf(stderr, "Magic number of %s language struct wrong\n",
	lang->la_name);
      abort();
    }

  if (!languages)
    {
      languages_allocsize = DEFAULT_ALLOCSIZE;
      languages = (const struct language_defn **) xmalloc
	(languages_allocsize * sizeof (*languages));
    }
  if (languages_size >= languages_allocsize)
    {
      languages_allocsize *= 2;
      languages = (const struct language_defn **) xrealloc ((char *) languages,
	languages_allocsize * sizeof (*languages));
    }
  languages[languages_size++] = lang;
}

/* Define the language that is no language.  */

static int
unk_lang_parser ()
{
  return 1;
}

static void
unk_lang_error (msg)
     char *msg;
{
  error ("Attempted to parse an expression with unknown language");
}

static struct type ** const (unknown_builtin_types[]) = { 0 };
static const struct op_print unk_op_print_tab[] = { 0 };

const struct language_defn unknown_language_defn = {
  "unknown",
  language_unknown,
  &unknown_builtin_types[0],
  range_check_off,
  type_check_off,
  unk_lang_parser,
  unk_lang_error,
  &builtin_type_error,		/* longest signed   integral type */
  &builtin_type_error,		/* longest unsigned integral type */
  &builtin_type_error,		/* longest floating point type */
  "0x%x", "0x%", "x",		/* Hex   format, prefix, suffix */
  "0%o",  "0%",  "o",		/* Octal format, prefix, suffix */
  unk_op_print_tab,		/* expression operators for printing */
  LANG_MAGIC
};

/* These two structs define fake entries for the "local" and "auto" options. */
const struct language_defn auto_language_defn = {
  "auto",
  language_auto,
  &unknown_builtin_types[0],
  range_check_off,
  type_check_off,
  unk_lang_parser,
  unk_lang_error,
  &builtin_type_error,		/* longest signed   integral type */
  &builtin_type_error,		/* longest unsigned integral type */
  &builtin_type_error,		/* longest floating point type */
  "0x%x", "0x%", "x",		/* Hex   format, prefix, suffix */
  "0%o",  "0%",  "o",		/* Octal format, prefix, suffix */
  unk_op_print_tab,		/* expression operators for printing */
  LANG_MAGIC
};

const struct language_defn local_language_defn = {
  "local",
  language_auto,
  &unknown_builtin_types[0],
  range_check_off,
  type_check_off,
  unk_lang_parser,
  unk_lang_error,
  &builtin_type_error,		/* longest signed   integral type */
  &builtin_type_error,		/* longest unsigned integral type */
  &builtin_type_error,		/* longest floating point type */
  "0x%x", "0x%", "x",		/* Hex   format, prefix, suffix */
  "0%o",  "0%",  "o",		/* Octal format, prefix, suffix */
  unk_op_print_tab,		/* expression operators for printing */
  LANG_MAGIC
};

/* Initialize the language routines */

void
_initialize_language()
{
   struct cmd_list_element *set, *show;

   /* GDB commands for language specific stuff */

   set = add_set_cmd ("language", class_support, var_string_noescape,
		      (char *)&language,
		      "Set the current source language.",
		      &setlist);
   show = add_show_from_set (set, &showlist);
   set->function.cfunc = set_language_command;
   show->function.cfunc = show_language_command;

   add_prefix_cmd ("check", no_class, set_check,
		   "Set the status of the type/range checker",
		   &setchecklist, "set check ", 0, &setlist);
   add_alias_cmd ("c", "check", no_class, 1, &setlist);
   add_alias_cmd ("ch", "check", no_class, 1, &setlist);

   add_prefix_cmd ("check", no_class, show_check,
		   "Show the status of the type/range checker",
		   &showchecklist, "show check ", 0, &showlist);
   add_alias_cmd ("c", "check", no_class, 1, &showlist);
   add_alias_cmd ("ch", "check", no_class, 1, &showlist);

   set = add_set_cmd ("type", class_support, var_string_noescape,
		      (char *)&type,
		      "Set type checking.  (on/warn/off/auto)",
		      &setchecklist);
   show = add_show_from_set (set, &showchecklist);
   set->function.cfunc = set_type_command;
   show->function.cfunc = show_type_command;

   set = add_set_cmd ("range", class_support, var_string_noescape,
		      (char *)&range,
		      "Set range checking.  (on/warn/off/auto)",
		      &setchecklist);
   show = add_show_from_set (set, &showchecklist);
   set->function.cfunc = set_range_command;
   show->function.cfunc = show_range_command;

   add_language (&unknown_language_defn);
   add_language (&local_language_defn);
   add_language (&auto_language_defn);

   language = savestring ("auto",strlen("auto"));
   range = savestring ("auto",strlen("auto"));
   type = savestring ("auto",strlen("auto"));

   /* Have the above take effect */

   set_language_command (language, 0);
   set_type_command (NULL, 0);
   set_range_command (NULL, 0);
}
