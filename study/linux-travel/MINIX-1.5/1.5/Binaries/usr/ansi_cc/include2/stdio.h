/*
 * stdio.h - input/output definitions
 *
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header: stdio.h,v 1.3 89/12/18 14:00:10 eck Exp $ */

#ifndef	_STDIO_H
#define	_STDIO_H

/*
 * Focus point of all stdio activity.
 */
typedef struct __iobuf {
	int		_count;
	int		_fd;
	int		_flags;
	int		_bufsiz;
	char		*_tname;
	unsigned char	*_buf;
	unsigned char	*_ptr;
} FILE;

#define	_IOFBF		0x000
#define	_IOREAD		0x001
#define	_IOWRITE	0x002
#define	_IONBF		0x004
#define	_IOMYBUF	0x008
#define	_IOEOF		0x010
#define	_IOERR		0x020
#define	_IOLBF		0x040
#define	_IOREADING	0x080
#define	_IOWRITING	0x100
#define	_IOAPPEND	0x200

/* The following definitions are also in <unistd.h>. They should not
 * conflict.
 */
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2

#define	stdin		(&__stdin)
#define	stdout		(&__stdout)
#define	stderr		(&__stderr)

#define	BUFSIZ		1024
#define	NULL		((void *)0)
#define	EOF		(-1)

#define	FOPEN_MAX	20

#define	FILENAME_MAX	14
#define	TMP_MAX		999
#define	L_tmpnam	(sizeof("/tmp/") + 15)

typedef long int	fpos_t;

#ifndef _SIZE_T
#define	_SIZE_T
typedef unsigned int	size_t;		/* type returned by sizeof */
#endif	/* _SIZE_T */

extern FILE	*__iotab[FOPEN_MAX];
extern FILE	__stdin, __stdout, __stderr;

#ifndef	_ANSI_H
#include	<ansi.h>
#endif

_PROTOTYPE( int remove, (const char *_filename)				);
_PROTOTYPE( int rename, (const char *_old, const char *_new)		);
_PROTOTYPE( FILE *tmpfile, (void)					);
_PROTOTYPE( char *tmpnam, (char *_s)					);
_PROTOTYPE( int fclose, (FILE *_stream)					);
_PROTOTYPE( int fflush, (FILE *_stream)					);
_PROTOTYPE( FILE *fopen, (const char *_filename, const char *_mode)	);
_PROTOTYPE( FILE *freopen,
	    (const char *_filename, const char *_mode, FILE *_stream)	);
_PROTOTYPE( void setbuf, (FILE *_stream, char *_buf)			);
_PROTOTYPE( int setvbuf,
		(FILE *_stream, char *_buf, int _mode, size_t _size)	);
_PROTOTYPE( int fprintf, (FILE *_stream, const char *_format, ...)	);
_PROTOTYPE( int fscanf, (FILE *_stream, const char *_format, ...)	);
_PROTOTYPE( int printf, (const char *_format, ...)			);
_PROTOTYPE( int scanf, (const char *_format, ...)			);
_PROTOTYPE( int sprintf, (char *_s, const char *_format, ...)		);
_PROTOTYPE( int sscanf, (char *_s, const char *_format, ...)		);
_PROTOTYPE( int vfprintf,
		(FILE *_stream, const char *_format, char *_arg)	);
_PROTOTYPE( int vprintf, (const char *_format, char *_arg)		);
_PROTOTYPE( int vsprintf, (char *_s, const char *_format, char *_arg)	);
_PROTOTYPE( int fgetc, (FILE *_stream)					);
_PROTOTYPE( char *fgets, (char *_s, int _n, FILE *_stream)		);
_PROTOTYPE( int fputc, (int _c, FILE *_stream)				);
_PROTOTYPE( int fputs, (const char *_s, FILE *_stream)			);
_PROTOTYPE( int getc, (FILE *_stream)					);
_PROTOTYPE( int getchar, (void)						);
_PROTOTYPE( char *gets, (char *_s)					);
_PROTOTYPE( int putc, (int _c, FILE *_stream)				);
_PROTOTYPE( int putchar, (int _c)					);
_PROTOTYPE( int puts, (const char *_s)					);
_PROTOTYPE( int ungetc, (int _c, FILE *_stream)				);
_PROTOTYPE( size_t fread,
	    (void *_ptr, size_t _size, size_t _nmemb, FILE *_stream)	);
_PROTOTYPE( size_t fwrite,
	(const void *_ptr, size_t _size, size_t _nmemb, FILE *_stream)	);
_PROTOTYPE( int fgetpos, (FILE *_stream, fpos_t *_pos)			);
_PROTOTYPE( int fseek, (FILE *_stream, long _offset, int _whence)	);
_PROTOTYPE( int fsetpos, (FILE *_stream, fpos_t *_pos)			);
_PROTOTYPE( long ftell, (FILE *_stream)					);
_PROTOTYPE( void rewind, (FILE *_stream)				);
_PROTOTYPE( void clearerr, (FILE *_stream)				);
_PROTOTYPE( int feof, (FILE *_stream)					);
_PROTOTYPE( int ferror, (FILE *_stream)					);
_PROTOTYPE( void perror, (const char *_s)				);
_PROTOTYPE( int __fillbuf, (FILE *_stream)				);
_PROTOTYPE( int __flushbuf, (int _c, FILE *_stream)			);

#define	getchar()	getc(stdin)
#define	putchar(c)	putc(c,stdout)
#define	getc(p)		(--(p)->_count >= 0 ? (int) (*(p)->_ptr++) : \
				__fillbuf(p))
#define	putc(c, p)	(--(p)->_count >= 0 ? \
			 (int) (*(p)->_ptr++ = (c)) : \
			 __flushbuf((c),(p)))

#define	feof(p)		(((p)->_flags & _IOEOF) != 0)
#define	ferror(p)	(((p)->_flags & _IOERR) != 0)

#ifdef _POSIX_SOURCE
_PROTOTYPE( int fileno, (FILE *_stream)					);
#define	fileno(stream)		((stream)->_fd)
#endif

#endif	/* _STDIO_H */
