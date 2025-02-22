/* Definitions for Intel 386 running Interactive Unix System V.
   Specifically, this is for recent versions that support POSIX;
   for version 2.0.2, use configuration option i386-sysv instead.  */

/* Mostly it's like AT&T Unix System V. */

#include "i386v.h"

/* Use crt1.o, not crt0.o, as a startup file, and crtn.o as a closing file. */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC \
  "%{!shlib:%{posix:%{pg:mcrtp1.o%s}%{!pg:%{p:mcrtp1.o%s}%{!p:crtp1.o%s}}}\
   %{!posix:%{pg:mcrt1.o%s}%{!pg:%{p:mcrt1.o%s}%{!p:crt1.o%s}}\
   %{p:-L/lib/libp} %{pg:-L/lib/libp}}}\
   %{shlib:%{posix:crtp1.o%s}%{!posix:crt1.o%s}} crtbegin.o%s"
  
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

/* Library spec */
#undef LIB_SPEC
#define LIB_SPEC "%{posix:-lcposix} %{shlib:-lc_s} -lc -lg"

#if 0
/* This is apparently not true: ISC versions up to 3.0,at least, use
   the standard calling sequence in which the called function pops the
   extra arg.  */
/* caller has to pop the extra argument passed to functions that return
   structures. */

#undef RETURN_POPS_ARGS
#define RETURN_POPS_ARGS(FUNTYPE,SIZE)   \
  (TREE_CODE (FUNTYPE) == IDENTIFIER_NODE ? 0			\
   : (TARGET_RTD						\
      && (TYPE_ARG_TYPES (FUNTYPE) == 0				\
	  || (TREE_VALUE (tree_last (TYPE_ARG_TYPES (FUNTYPE)))	\
	      == void_type_node))) ? (SIZE)			\
   : 0)
/* On other 386 systems, the last line looks like this:
   : (aggregate_value_p (FUNTYPE)) ? GET_MODE_SIZE (Pmode) : 0)  */
#endif
