/* The <limits.h> header defines some basic sizes, both of the language types 
 * (e.g., the number of bits in an integer), and of the operating system (e.g.
 * the number of characters in a file name.
 */

#ifndef _LIMITS_H
#define _LIMITS_H

/* Definitions about chars (8 bits in MINIX, and signed). */
#define CHAR_BIT           8	/* # bits in a char */
#define CHAR_MIN        -128	/* minimum value of a char */
#define CHAR_MAX         127	/* maximum value of a char */
#define SCHAR_MIN       -128	/* minimum value of a signed char */
#define SCHAR_MAX        127	/* maximum value of a signed char */
#define UCHAR_MAX        255	/* maximum value of an unsigned char */
#define MB_LEN_MAX         1	/* maximum length of a multibyte char */

/* Definitions about shorts (16 bits in MINIX). */
#define SHRT_MIN  (-32767-1)	/* minimum value of a short */
#define SHRT_MAX       32767	/* maximum value of a short */
#define USHRT_MAX      65535	/* maximum value of unsigned short */

/* Definitions about ints (16 bits in MINIX for 8088, 80286, Atari etc) */
#define INT_MIN   (-32767-1)	/* minimum value of an int */
#define INT_MAX        32767	/* maximum value of an int */
#define UINT_MAX       65535	/* maximum value of an unsigned int */

/*Definitions about longs (32 bits in MINIX). */
#define LONG_MIN (-2147483647-1)/* minimum value of a long */
#define LONG_MAX  2147483647L	/* maximum value of a long */
#define ULONG_MAX 4294967295L	/* maximum value of an unsigned long */

/* Minimum sizes required by the POSIX P1003.1 standard (Table 2-2). */
#ifdef _POSIX_SOURCE		/* these are only visible for POSIX */
#define _POSIX_ARG_MAX  4096	/* exec() may have 4K worth of args */
#define _POSIX_CHILD_MAX   6	/* a process may have 6 children */
#define _POSIX_LINK_MAX    8	/* a file may have 8 links */
#define _POSIX_MAX_CANON 255	/* size of the canonical input queue */
#define _POSIX_MAX_INPUT 255	/* you can type 255 chars ahead */
#define _POSIX_NAME_MAX   14	/* a file name may have 14 chars */
#define _POSIX_NGROUPS_MAX 0	/* supplementary group IDs are optional */
#define _POSIX_OPEN_MAX   16	/* a process may have 16 files open */
#define _POSIX_PATH_MAX  255	/* a pathname may contain 255 chars */
#define _POSIX_PIPE_BUF  512	/* pipes writes of 512 bytes are atomic */

/* Values actually implemented by MINIX (Tables 2-3, 2-4, and 2-5). */
/* Some of these old names had better be defined when not POSIX. */
#define _NO_LIMIT        100	/* arbitrary number; limit not enforced */

#define NGROUPS_MAX        0	/* supplemental group IDs not available */
#define ARG_MAX         4096	/* # bytes of args + environ for exec() */
#define CHILD_MAX  _NO_LIMIT    /* MINIX does not limit children */
#define OPEN_MAX          20	/* # open files a process may have */
#define LINK_MAX         127	/* # links a file may have */
#define MAX_CANON        255	/* size of the canonical input queue */
#define MAX_INPUT        255	/* size of the type-ahead buffer */
#define NAME_MAX          14	/* # chars in a file name */
#define PATH_MAX         255	/* # chars in a path name */
#define PIPE_BUF         512	/* # bytes in atomic write to a pipe */

#endif /* _POSIX_SOURCE */

#endif /* _LIMITS_H */
