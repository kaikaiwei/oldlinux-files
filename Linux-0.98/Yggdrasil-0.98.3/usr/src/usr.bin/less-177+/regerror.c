#include <stdio.h>

void
regerror(s)
char *s;
{
#ifdef ERRAVAIL
	error("regexp: %s", &s);
#else
	fprintf(stderr, "regexp(3): %s", s);
	exit(1);
#endif
	/* NOTREACHED */
}
