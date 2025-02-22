/* lex.c: rc's lexical analyzer */

#include "rc.h"
#include "lex.h"
#include "y.tab.h"
#include "nalloc.h"
#include "input.h"
#include "utils.h"
#include "hash.h"
#include "heredoc.h"

/*
	Special characters (i.e., "non-word") in rc:
		\t \n # ; & | ^ $ = ~ ` ' { } @ ! ( ) < > \

	The lexical analyzer is fairly straightforward. The only really unclean part
	concerns backslash continuation and "double backslashes". A backslash followed by
	a newline is treated as a space, otherwise backslash is not a special characeter
	(i.e., it can be part of a word).  This introduces a host of unwanted special
	cases. In our case, \ cannot be a word character, since we wish to read in all
	word characters in a tight loop.

	Note: to save the trouble of declaring these arrays with TRUEs and FALSEs, I am assuming
	that FALSE = 0, TRUE = 1. (and so is it declared in rc.h)
*/

#define BUFSIZE ((SIZE_T) 1000)	/*	malloc hates power of 2 buffers? */
#define BUFMAX (8 * BUFSIZE)	/* 	How big the buffer can get before we re-allocate the
					space at BUFSIZE again. Premature optimization? Maybe.
				*/

enum wordstates { NW, RW, KW }; /* "nonword", "realword", "keyword" */

static void getpair(int);

int lineno;

const char nw[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char dnw[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static SIZE_T bufsize = BUFSIZE;
static char *realbuf = NULL;
static boolean newline = FALSE;
static boolean errset = FALSE;
static boolean prerror = FALSE;
static enum wordstates word = NW;
static int fd_left, fd_right;

#define checkfreecaret {if (word != NW) { word = NW; ugchar(c); return '^'; }}

enum filedescriptors { UNSET = -9, CLOSED = -1 };

int yylex() {
	static boolean dollar = FALSE;
	boolean saw_meta = FALSE;
	int c;
	SIZE_T i;			/* The purpose of all these local assignments is to	*/
	const char *meta;		/* allow optimizing compilers like gcc to load these	*/
	char *buf = realbuf;		/* values into registers. On a sparc this is a		*/
	YYSTYPE *y = &yylval;		/* win, in code size *and* execution time		*/

	if (errset) {
		errset = FALSE;
		return '\n';
	}

	/* rc variable-names may contain only alnum, '*' and '_', so use dnw if we are scanning one. */
	meta = (dollar ? dnw : nw);
	dollar = FALSE;

	if (newline) {
		--lineno; /* slight space optimization; print_prompt2() always increments lineno */
		print_prompt2();
		newline = FALSE;
	}

top:	while ((c = gchar()) == ' ' || c == '\t')
		word = NW;

	if (c == EOF)
		return END;

	if (!meta[(unsigned char) c]) {	/* it's a word or keyword. */
		checkfreecaret;
		word = RW;
		i = 0;
	read:	do {
			buf[i++] = c;
			if (c == '?' || c == '[' || c == '*')
				saw_meta = TRUE;
			if (i >= bufsize)
				buf = realbuf = erealloc(buf, bufsize *= 2);
		} while ((c = gchar()) != EOF && !meta[(unsigned char) c]);
		if (c == '\\') {
	anotherbs:	if ((c = gchar()) == '\n') {
				print_prompt2();
				c = ' '; /* Pretend a space was read */
			} else {
	bs:			buf[i++] = '\\';
				if (i >= bufsize)
					buf = realbuf = erealloc(buf, bufsize *= 2);
				if (!meta[(unsigned char) c])
					goto read;
				if (c == '\\')
					goto anotherbs;
			}
		}
		ugchar(c);
		buf[i] = '\0';
		word = KW;
		if (i == 2) {
			if (*buf == 'i' && buf[1] == 'f') return IF;
			if (*buf == 'f' && buf[1] == 'n') return FN;
			if (*buf == 'i' && buf[1] == 'n') return IN;
		}
		if (streq(buf, "for")) return FOR;
		if (streq(buf, "else")) return ELSE;
		if (streq(buf, "switch")) return SWITCH;
		if (streq(buf, "while")) return WHILE;
		if (streq(buf, "case")) return CASE;
		word = RW;
		y->word.w = ncpy(buf);
		if (saw_meta) {
			char *r, *s;

			y->word.m = nalloc(strlen(buf) + 1);
			for (r = buf, s = y->word.m; *r != '\0'; r++, s++)
				*s = (*r == '?' || *r == '[' || *r == '*');
		} else {
			y->word.m = NULL;
		}
		return WORD;
	}

	if (c == '`' || c == '!' || c == '@' || c == '~' || c == '$' || c == '\'') {
		checkfreecaret;
		if (c == '!' || c == '@' || c == '~')
			word = KW;
	}

	switch (c) {
	case '\0':
		pr_error("warning: null character ignored");
		goto top;
	case '!':
		return BANG;
	case '@':
		return SUBSHELL;
	case '~':
		return TWIDDLE;
	case '`':
		c = gchar();
		if (c == '`')
			return BACKBACK;
		ugchar(c);
		return '`';
	case '$':
		dollar = TRUE;
		c = gchar();
		if (c == '#')
			return COUNT;
		if (c == '^')
			return FLAT;
		ugchar(c);
		return '$';
	case '\'':
		word = RW;
		i = 0;
		do {
			buf[i++] = c;
			if (c == '\n')
				print_prompt2();
			if (c == EOF) {
				word = NW;
				scanerror("eof in quoted string");
				return HUH;
			}
			if (i >= bufsize)
				buf = realbuf = erealloc(buf, bufsize *= 2);
		} while ((c = gchar()) != '\'' || (c = gchar()) == '\''); /* quote "'" thus: 'how''s it going?' */
		ugchar(c);
		buf[i] = '\0';
		y->word.w = ncpy(buf);
		y->word.m = NULL;
		return WORD;
	case '\\':
		if ((c = gchar()) == '\n') {
			print_prompt2();
			goto top; /* Pretend it was just another space. */
		}
		ugchar(c);
		c = '\\';
		checkfreecaret;
		c = gchar();
		i = 0;
		goto bs;
	case '(':
		if (word == RW) /* SUB's happen only after real words, not keyowrds, so if () and while () work */
			c = SUB;
		word = NW;
		return c;
	case '#':
		while ((c = gchar()) != '\n') /* skip comment until newline */
			if (c == EOF)
				return END;
		/* FALLTHROUGH */
	case '\n':
		lineno++;
		newline = TRUE;
		/* FALLTHROUGH */
	case ';':
	case '^':
	case ')':
	case '=':
	case '{': case '}':
		word = NW;
		return c;
	case '&':
		word = NW;
		c = gchar();
		if (c == '&')
			return ANDAND;
		ugchar(c);
		return '&';
	case '|':
		word = NW;
		c = gchar();
		if (c == '|')
			return OROR;
		getpair(c);
		if (errset)
			return HUH;
		if ((y->pipe.left = fd_left) == UNSET)
			y->pipe.left = 1;				/* default to fd 1 */
		if ((y->pipe.right = fd_right) == UNSET)
			y->pipe.right = 0;				/* default to fd 0 */
		if (y->pipe.right == CLOSED) {
			scanerror("expected digit after '='");		/* can't close a pipe */
			return HUH;
		}
		return PIPE;
	case '>':
		c = gchar();
		if (c == '>') {
			c = gchar();
			y->redir.type = APPEND;
		} else
			y->redir.type = CREATE;
		y->redir.fd = 1;
		goto common;
	case '<':
		c = gchar();
		if (c == '<') {
			c = gchar();
			if (c == '<') {
				c = gchar();
				y->redir.type = HERESTRING;
			} else {
				y->redir.type = HEREDOC;
			}
		} else
			y->redir.type = FROM;
		y->redir.fd = 0;
	common:
		word = NW;
		getpair(c);
		if (errset)
			return HUH;
		if (fd_right == UNSET) { /* redirection, not dup */
			if (fd_left != UNSET)
				y->redir.fd = fd_left;
			return REDIR;
		} else { /* dup; recast yylval */
			y->dup.type = y->redir.type;
			y->dup.left = fd_left;
			y->dup.right = fd_right;
			return DUP;
		}
	default:
		word = NW;
		return c; /* don't know what it is, let yacc barf on it */
	}
}

void yyerror(const char *s) {
	char *tok;
	char tokbuf[128];

	if (prerror) { /* don't print "syntax error" if there's a more informative scanerror */
		prerror = FALSE;
		return;
	}

	if (!interactive) {
		if (word != NW)
			tok = realbuf;
		else if (last == EOF)
			tok = "end of input";
		else if (last == '\n')
			tok = "end of line";
		else
			sprint(tok = tokbuf, (last < 32 || last > 126) ? "(decimal %d)" : "'%c'",last);
		fprint(2,"line %d: %s near %s\n", lineno - (last == '\n'), s, tok);
	} else
		fprint(2,"%s\n",s);
}

void scanerror(char *s) {
	flushu(); /* flush upto newline */
	yyerror(s);
	errset = prerror = TRUE;
}

void inityy() {
	newline = FALSE;
	word = NW;
	hq = NULL;

	/* return memory to the system if the buffer got too large */

	if (bufsize > BUFMAX && realbuf != NULL) {
		efree(realbuf);
		bufsize = BUFSIZE;
		realbuf = ealloc(bufsize);
	} else if (realbuf == NULL)
		realbuf = ealloc(bufsize);
}

void print_prompt2() {
	lineno++;
#ifdef READLINE
	prompt = prompt2;
#else
	if (interactive)
		fprint(2,"%s",prompt2);
#endif
}

/*
   Scan in a pair of integers for redirections like >[2=1]. CLOSED represents a closed file
   descriptor (i.e., >[2=]) and UNSET represents an undesignated file descriptor (e.g.,
   >[2] is represented as (2,UNSET).

   This function makes use of unsigned compares to make range tests in one compare operation.
*/

static void getpair(int c) {
	int n;

	fd_left = fd_right = UNSET;

	if (c != '[') {
		ugchar(c);
		return;
	}

	if ((unsigned int) (n = gchar() - '0') > 9) {
		scanerror("expected digit after '['");
		return;
	}

	while ((unsigned int) (c = gchar() - '0') <= 9)
		n = n * 10 + c;

	fd_left = n;
	c += '0';

	switch (c) {
	default:
		scanerror("expected '=' or ']' after digit");
		return;
	case ']':
		return;
	case '=':
		if ((unsigned int) (n = gchar() - '0') > 9) {
			if (n != ']' - '0') {
				scanerror("expected digit or ']' after '='");
				return;
			}
			fd_right = CLOSED;
		} else {
			while ((unsigned int) (c = gchar() - '0') <= 9)
				n = n * 10 + c;
			if (c != ']' - '0') {
				scanerror("expected ']' after digit");
				return;
			}
			fd_right = n;
		}
	}
}
